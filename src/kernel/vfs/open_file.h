#pragma once

#include "../util/ref.h"
#include "vnode.h"

typedef struct open_file_descriptor open_file_descriptor_t;
typedef struct open_file_descriptor
{
    int flags;
    vnode_ref_t vnode;
    struct stat st;
    off_t offset;
    spinlock_noint_t lock;
    struct ref ref;

    ssize_t (*read)(open_file_descriptor_t*, void*, size_t); 
    ssize_t (*write)(open_file_descriptor_t*, void*, size_t);
} open_file_descriptor_t;

typedef struct
{
    open_file_descriptor_t* _Atomic ptr;
} open_file_descriptor_ref_t;

typedef struct
{
    int flags; // FD_CLOEXEC
    open_file_descriptor_ref_t fd;
} open_file_descriptor_entry_t;

#define OPEN_FD_REF_INIT ((struct ref){___open_file_descriptor_free, 1})

void ___open_file_descriptor_free(const struct ref* ref);
void open_file_descriptor_delete_ref(open_file_descriptor_ref_t* ref);
open_file_descriptor_ref_t open_file_descriptor_copy_ref(open_file_descriptor_ref_t ref);

open_file_descriptor_ref_t vfs_allocate_new_open_file_descriptor(int flags, vnode_ref_t vnode, struct stat* st);
void vfs_free_open_file_descriptor(int ofd);

int vfs_allocate_thread_fd(open_file_descriptor_ref_t desc, int flags);
bool vfs_get_thread_fd(int fd, open_file_descriptor_ref_t* desc, int* flags);
int vfs_open(const char* path, int flags, mode_t mode,
             uid_t euid, gid_t egid,
             vnode_ref_t root, vnode_ref_t cwd,
             mode_t umask);
int vfs_close(int fd);
void vfs_close_all();

void vfs_log_thread_fds();
