#pragma once

#include "inttypes.h"
#include "sys/types.h"

#define STDIN_FILENO    0
#define STDOUT_FILENO   1
#define STDERR_FILENO   2

#include "seek_defs.h"

pid_t getpid();
pid_t fork();
ssize_t write(int fildes, const void *buf, size_t nbyte);
ssize_t read(int fildes, void *buf, size_t nbyte);
int close(int fildes);
void *sbrk(intptr_t incr);
off_t lseek(int fildes, off_t offset, int whence);