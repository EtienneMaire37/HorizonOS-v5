#pragma once

typedef struct inode inode_t;

typedef struct
{
    inode_t* _Atomic ptr;
} inode_ref_t;
