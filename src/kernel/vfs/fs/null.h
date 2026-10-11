#pragma once

#include <sys/types.h>
#include "../vnode.h"

ssize_t null_read(vnode_ref_t node, void* buf, size_t count, off_t off);
ssize_t null_write(vnode_ref_t node, void* buf, size_t count, off_t off);
