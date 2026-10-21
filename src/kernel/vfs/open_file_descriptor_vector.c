#include "open_file_descriptor_vector.h"
#include "../util/assert.h"
#include "abi-bits/fcntl.h"
#include "inode.h"
#include "inode_stack.h"
#include "vnode.h"
#include "open_file.h"
#include "../multitasking/multitasking.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

int ofdv_open(open_file_descriptor_vector_t* vec, open_file_descriptor_entry_t entry)
{
    ASSERT(entry.fd.ptr);
    uint32_t flags = acquire_spinlock_noint(&vec->lock);
    for (size_t i = 0; i < vec->size; i++)
    {
        if (!vec->data[i].fd.ptr)
        {
            open_file_descriptor_copy_ref(entry.fd);
            vec->data[i] = entry;
            release_spinlock_noint(&vec->lock, flags);
            return i;
        }
    }
    
    if (vec->size * sizeof(inode_ref_t) == vec->allocated)
    {
        if (vec->allocated == 0)
        // Most vnodes won't have multiple mountpoints on them so it'd be wasteful to use more than one
            vec->allocated = sizeof(inode_ref_t); 
        else
            vec->allocated *= 2;

        open_file_descriptor_entry_t* data = (open_file_descriptor_entry_t*)realloc(vec->data, vec->allocated);
        if (!data) return (release_spinlock_noint(&vec->lock, flags), -ENOMEM);
        vec->data = data;
    }

    open_file_descriptor_copy_ref(entry.fd);
    vec->data[vec->size] = entry;
    int idx = vec->size;
    vec->size++;
    release_spinlock_noint(&vec->lock, flags);
    
    return idx;
}
open_file_descriptor_entry_t ofdv_get(open_file_descriptor_vector_t* vec, int fd)
{
    if (fd < 0) return (open_file_descriptor_entry_t){ .flags = 0, .fd.ptr = NULL };
    uint32_t flags = acquire_spinlock_noint(&vec->lock);
    open_file_descriptor_entry_t ret = { .flags = 0, .fd.ptr = NULL };
    if ((size_t)fd <= vec->size)
        ret = vec->data[fd];
    release_spinlock_noint(&vec->lock, flags);
    open_file_descriptor_copy_ref(ret.fd);
    return ret;
}
int ofdv_close(open_file_descriptor_vector_t* vec, int fd)
{
    if (fd < 0) return EBADF;
    uint32_t flags = acquire_spinlock_noint(&vec->lock);
    int ret = (size_t)fd <= vec->size ? 0 : EBADF;
    if (!ret)
        open_file_descriptor_delete_ref(&vec->data[fd].fd);
    release_spinlock_noint(&vec->lock, flags);
    return ret;
}
void ofdv_closeall(open_file_descriptor_vector_t* vec)
{
    uint32_t flags = acquire_spinlock_noint(&vec->lock);
    for (size_t i = 0; i < vec->size; i++)
        open_file_descriptor_delete_ref(&vec->data[i].fd);
    free(vec->data);
    vec->data = NULL;
    vec->allocated = vec->size = 0;
    release_spinlock_noint(&vec->lock, flags);
}
open_file_descriptor_vector_t ofdv_copyall(open_file_descriptor_vector_t* vec, bool cloexec)
{
    open_file_descriptor_vector_t ret = { NULL , 0, 0, SPINLOCK_NOINT_INIT };
    uint32_t flags = acquire_spinlock_noint(&vec->lock);
    if (!(ret.data = malloc(vec->allocated)))
    {
        release_spinlock_noint(&vec->lock, flags);
        return ret;
    }
    memcpy(ret.data, vec->data, vec->size);
    ret.size = vec->size;
    ret.allocated = vec->allocated;
    for (size_t i = 0; i < vec->size; i++)
    {
        if (cloexec && (vec->data[i].flags & FD_CLOEXEC))
            vec->data[i].fd.ptr = NULL;
        else
            open_file_descriptor_copy_ref(vec->data[i].fd);
    }
    release_spinlock_noint(&vec->lock, flags);
    return ret;
}

int vfs_isatty(int fd)
{
    open_file_descriptor_entry_t entry = ofdv_get(&current_task->open_file_table, fd);
    int ret;
    if (entry.fd.ptr)
    {
        inode_ref_t inode = inode_stack_top_inode(&entry.fd.ptr->vnode.ptr->inodes);
        ret = inode.ptr ? (inode.ptr->isatty ? 0 : ENOTTY) : ENOTTY;
        inode_delete_ref(&inode);
    }
    else
        ret = EBADF;
    open_file_descriptor_delete_ref(&entry.fd);
    return ret;
}

int vfs_dup(int fd)
{
    open_file_descriptor_entry_t entry = ofdv_get(&current_task->open_file_table, fd);
    int ret = ofdv_open(&current_task->open_file_table, entry);
    open_file_descriptor_delete_ref(&entry.fd);
    return ret;
}
int vfs_dup3(int oldfd, int newfd, int flags)
{
    if (oldfd < 0 || newfd < 0) return EBADF;
    if (oldfd == newfd) return EINVAL;
    open_file_descriptor_entry_t entry = ofdv_get(&current_task->open_file_table, oldfd);
    if (!entry.fd.ptr) return EBADF;
    uint32_t eflags = acquire_spinlock_noint(&current_task->open_file_table.lock);

    if ((size_t)newfd >= current_task->open_file_table.size)
    {
        size_t oldsize = current_task->open_file_table.size;
        size_t newsize = newfd + 1;
        current_task->open_file_table.size = newsize;
        if (newsize > current_task->open_file_table.allocated)
        {
            open_file_descriptor_entry_t* data = (open_file_descriptor_entry_t*)realloc(current_task->open_file_table.data, current_task->open_file_table.allocated);
            if (!data) return (release_spinlock_noint(&current_task->open_file_table.lock, eflags), ENOMEM);
            current_task->open_file_table.data = data;
        }
        for (size_t i = oldsize; i < newsize - 1; i++)
            current_task->open_file_table.data[i].fd.ptr = NULL;
    }

    open_file_descriptor_delete_ref(&current_task->open_file_table.data[newfd].fd);
    current_task->open_file_table.data[newfd].fd = open_file_descriptor_copy_ref(entry.fd);
    current_task->open_file_table.data[newfd].flags = flags & O_CLOEXEC ? FD_CLOEXEC : 0;
    
    release_spinlock_noint(&current_task->open_file_table.lock, eflags);
    open_file_descriptor_delete_ref(&entry.fd);
    return 0;
}
