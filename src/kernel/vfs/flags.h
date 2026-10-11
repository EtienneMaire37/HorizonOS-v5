#pragma once

#include <stdint.h>

typedef uint8_t vnode_flags_t;

#define VNODE_INIT         0x00

#define VNODE_EXPLORED     0x01
#define VNODE_EXPLORING    0x02
