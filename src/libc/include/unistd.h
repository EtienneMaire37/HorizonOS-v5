#pragma once

#include "inttypes.h"
#include "sys/types.h"

#define STDIN_FILENO    0
#define STDOUT_FILENO   1
#define STDERR_FILENO   2

pid_t getpid();
pid_t fork();
ssize_t write(int fildes, const void *buf, size_t nbyte);
ssize_t read(int fildes, void *buf, size_t nbyte);
void *sbrk(intptr_t incr);