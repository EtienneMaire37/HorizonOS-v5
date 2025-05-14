#pragma once

int open(const char* path, int oflag, ...);
int close(int fildes);
ssize_t write(int fildes, const void *buf, size_t nbyte);
ssize_t read(int fildes, void *buf, size_t nbyte);
void exit(int r);
time_t time(time_t* t);
pid_t getpid();
int brk(void *addr);

#include "../include/syscall_defines.h"