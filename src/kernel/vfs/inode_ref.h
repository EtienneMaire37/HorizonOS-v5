#pragma once

typedef struct inode inode_t;

typedef struct __attribute__((packed))
{
    inode_t* _Atomic ptr;
} inode_ref_t;
