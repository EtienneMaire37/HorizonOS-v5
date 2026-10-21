#include "mountpoint.h"
#include "mountpoint_ref.h"
#include "../util/ref.h"
#include "vnode.h"
#include <stdlib.h>

void mountpoint_delete_ref(mountpoint_ref_t* ref)
{
    if (!ref) return;
    mountpoint_ref_t ref_ref = (mountpoint_ref_t){ __atomic_exchange_n((void**)&ref->ptr, 0, __ATOMIC_RELEASE ) };
    struct ref* ref_ref_ref = ref_ref.ptr ? &ref_ref.ptr->ref : NULL;
    if (ref_ref_ref)
        ref_dec(ref_ref_ref);
}

mountpoint_ref_t mountpoint_copy_ref(mountpoint_ref_t ref)
{
    if (ref.ptr)
        ref_inc(&ref.ptr->ref);
    return ref;
}

void ___mountpoint_free(const struct ref* ref)
{
    mountpoint_t* mountpoint = container_of(ref, mountpoint_t, ref);
    void (*free_data)(void* data) = mountpoint->free_data;
    if (free_data)
        free_data(mountpoint->data);
    vnode_delete_ref(&mountpoint->root);
    vnode_delete_ref(&mountpoint->dev_node);
    free(mountpoint);
}
