#pragma once

ssize_t write(int fildes, const void *buf, size_t nbyte);
ssize_t read(int fildes, void *buf, size_t nbyte);
void exit(int r);
time_t time(time_t* t);
pid_t getpid();

#define SYSCALL_EXIT    0
#define SYSCALL_TIME    1
#define SYSCALL_READ    2
#define SYSCALL_WRITE   3
#define SYSCALL_GETPID  4
#define SYSCALL_FORK    5