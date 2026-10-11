#pragma once

typedef struct vnode vnode_t;

typedef struct __attribute__((packed))
{
    vnode_t* _Atomic ptr;
} vnode_ref_t;
