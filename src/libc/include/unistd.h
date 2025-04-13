#pragma once

#include "inttypes.h"
#include "sys/types.h"

#define STDIN_FILENO    0
#define STDOUT_FILENO   1
#define STDERR_FILENO   2

// void* sbrk(intptr_t increment);
pid_t getpid();
pid_t fork();