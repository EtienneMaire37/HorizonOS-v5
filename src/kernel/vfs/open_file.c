#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include "open_file.h"
#include "inode_stack.h"
#include "mountpoint.h"
#include "ref.h"
#include "vnode.h"
#include "vnode_ref.h"
#include "spinlock.h"
#include "util/assert.h"

/* STUB */
// TODO: Use rw locks everywhere it should be used
static spinlock_noint_t lock;
#define OFDT_ENTRIES   64
static open_file_descriptor_entry_t entries[OFDT_ENTRIES];
/* STUB */

void open_file_descriptor_delete_ref(open_file_descriptor_ref_t* ref)
{
    if (!ref || !ref->ptr) return;
    struct ref* ref_ref = ref->ptr ? &ref->ptr->ref : NULL;
    ref->ptr = NULL;
    if (ref_ref)
        ref_dec(ref_ref);
}

open_file_descriptor_ref_t open_file_descriptor_copy_ref(open_file_descriptor_ref_t ref)
{
    if (!ref.ptr) return ref;
    ref_inc(&ref.ptr->ref);
    return ref;
}

open_file_descriptor_ref_t vfs_allocate_new_open_file_descriptor(int flags, vnode_ref_t vnode, struct stat *st)
{
    open_file_descriptor_ref_t ref = { calloc(1, sizeof(open_file_descriptor_t)) };
    if (!ref.ptr) return ref;
    ref.ptr->ref = OPEN_FD_REF_INIT;
    ref.ptr->st = *st;
    ref.ptr->vnode = vnode_copy_ref(vnode);
    ref.ptr->lock = SPINLOCK_NOINT_INIT;
    ref.ptr->flags = flags;
    ref.ptr->offset = 0;
    // TODO: Make a wrapper around vnode's io functions
    // or just design this better
    return ref;
}

open_file_descriptor_ref_t vfs_create_new_file_descriptor_from_vnode(int* _errno, int flags, vnode_ref_t vnode)
{
    ASSERT(_errno);
    ASSERT(vnode.ptr);
    *_errno = 0;
    struct stat st = vnode_stat(vnode);
    open_file_descriptor_ref_t ofd = vfs_allocate_new_open_file_descriptor(flags, vnode, &st);
    if (!ofd.ptr)
    {
        *_errno = ENOMEM;
        return ofd;
    }
    mountpoint_ref_t mp = inode_stack_top_mountpoint(&vnode.ptr->inodes);
    uint32_t eflags = acquire_spinlock_noint(&mp.ptr->lock);
    bool unmounting = mp.ptr->unmounting;
    if (!unmounting)
        mp.ptr->busy++;
    release_spinlock_noint(&mp.ptr->lock, eflags);
    mountpoint_delete_ref(&mp);
    if (unmounting)
    {
        open_file_descriptor_delete_ref(&ofd);
        *_errno = ENOENT;
        return ofd;
    }
    return ofd;
}

void ___open_file_descriptor_free(const struct ref *ref)
{
    open_file_descriptor_t* ofd = container_of(ref, open_file_descriptor_t, ref);
    if (ofd->vnode.ptr)
    {
        mountpoint_ref_t mp = inode_stack_top_mountpoint(&ofd->vnode.ptr->inodes);
        uint32_t eflags = acquire_spinlock_noint(&mp.ptr->lock);
        mp.ptr->busy--;
        release_spinlock_noint(&mp.ptr->lock, eflags);
        mountpoint_delete_ref(&mp);
        vnode_delete_ref(&ofd->vnode);
    }
    free(ofd);
}

int vfs_allocate_thread_fd(open_file_descriptor_ref_t desc, int flags)
{
    int fd = -1;
    uint32_t eflags = acquire_spinlock_noint(&lock);
    for (int i = 0; i < OFDT_ENTRIES; i++)
    {
        if (entries[i].fd.ptr == NULL)
        {
            entries[i].fd.ptr = open_file_descriptor_copy_ref(desc).ptr;
            entries[i].flags = flags;
            fd = i;
            goto end;
        }
    }
end:
    release_spinlock_noint(&lock, eflags);
    return fd;
}
bool vfs_get_thread_fd(int fd, open_file_descriptor_ref_t* desc, int* flags)
{
    ASSERT(desc);
    ASSERT(flags);
    if (fd < 0 || fd >= OFDT_ENTRIES) return false;
    uint32_t eflags = acquire_spinlock_noint(&lock);
    *desc = open_file_descriptor_copy_ref(entries[fd].fd);
    *flags = entries[fd].flags;
    release_spinlock_noint(&lock, eflags);
    return true;
}

int vfs_close(int fd)
{
    if (fd < 0 || fd >= OFDT_ENTRIES) return EBADF;
    int ret = 0;
    uint32_t eflags = acquire_spinlock_noint(&lock);
    if (entries[fd].fd.ptr)
        open_file_descriptor_delete_ref(&entries[fd].fd);
    else
        ret = EBADF;
    release_spinlock_noint(&lock, eflags);
    return ret;
}
void vfs_close_all()
{
    for (int i = 0; i < OFDT_ENTRIES; i++)
        vfs_close(i);
}
int vfs_open(const char* path, int flags, mode_t mode,
             uid_t euid, gid_t egid,
             vnode_ref_t root, vnode_ref_t cwd,
             mode_t umask)
{
    ASSERT(root.ptr); // Should always be the root of the current process (probably)
    ASSERT(cwd.ptr);  // Should always be the current working directory of the current process (not null)

    // Not implemented
    if (flags & ~(O_NOFOLLOW | O_DIRECTORY | O_CLOEXEC)) return EINVAL;

    // Invalid flag combinations
    if ((flags & O_EXCL) && !(flags & O_CREAT)) return EINVAL;

    // TODO: Implement ACLs
    mode &= ~umask;
    
    int _errno;
    vnode_ref_t vnode = vfs_get_vnode_from_path(&_errno, euid, egid, path, root, cwd, !(flags & O_NOFOLLOW));

    int ret = -ENOSYS;
    if (!vnode.ptr)
    {
        if (_errno == ENOENT && (flags & O_CREAT))
        {
            // TODO: Implement file creation
        }
        ret = -_errno;
        goto end;
    }
    else
    {
        mode = vnode_stat(vnode).st_mode;
        if ((flags & O_DIRECTORY) && !S_ISDIR(mode))
        {
            ret = -ENOTDIR;
            goto end;
        }
        if ((flags & O_CREAT) && (flags & O_EXCL))
        {
            ret = -EEXIST;
            goto end;
        }
        if ((flags & O_TRUNC) && S_ISREG(mode))
        {
            // TODO: trunc
        }
    }
    open_file_descriptor_ref_t desc = vfs_create_new_file_descriptor_from_vnode(&_errno, flags, vnode);
    if (!desc.ptr)
    {
        ret = -_errno;
        goto end;
    }
    ret = vfs_allocate_thread_fd(desc, (flags & O_CLOEXEC) ? FD_CLOEXEC : 0);
    open_file_descriptor_delete_ref(&desc);
    if (ret == -1) ret = -EMFILE;
end:
    vnode_delete_ref(&vnode);
    return ret;
}

void vfs_log_thread_fds()
{
    // No need to lock as it is only a stub for debugging the user space build (same as everywhere else)
    for (int i = 0; i < OFDT_ENTRIES; i++)
    {
        if (entries[i].fd.ptr)
        {
            LOG(DEBUG, "fd %d: ", i);
            LOG(DEBUG, "- flags: %#o", entries[i].flags);
            LOG(DEBUG, "- mode:  %#o", entries[i].fd.ptr->st.st_mode);
        }
    }
}
