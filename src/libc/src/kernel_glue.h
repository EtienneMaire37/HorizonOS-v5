#pragma once

ssize_t write(int fildes, const void *buf, size_t nbyte);
void exit(int r);
time_t time(time_t* t);
pid_t getpid();
void* sbrk(intptr_t increment);