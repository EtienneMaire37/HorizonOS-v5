#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include "open_file.h"
#include "inode_stack.h"
#include "mountpoint.h"
#include "../util/ref.h"

#include "vnode.h"
#include "vnode_ref.h"
#include "../multicore/spinlock.h"
#include "../util/assert.h"

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
    ASSERT(vnode.ptr);
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
