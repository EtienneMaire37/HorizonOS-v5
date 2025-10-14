#pragma once

#include "seek_defs.h"

#define O_CLOEXEC   0x1
#define O_CREAT     0x2
#define O_DIRECTORY 0x4
#define O_EXCL      0x8
#define O_NOCTTY    0x10
#define O_NOFOLLOW  0x20
#define O_TRUNC     0x40
#define O_TTY_INIT  0x80

#define O_APPEND    0x100
#define O_DSYNC     0x200
#define O_NONBLOCK  0x400
#define O_RSYNC     0x800
#define O_SYNC      0x1000

#define O_ACCMODE   0x2000

#define O_EXEC      0x4000
#define O_RDONLY    0x8000
#define O_RDWR      0x10000
#define O_SEARCH    0x20000
#define O_WRONLY    0x40000

int open(const char* path, int oflag, ...);