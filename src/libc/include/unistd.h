#pragma once

#include "inttypes.h"
#include "sys/types.h"

void* sbrk(intptr_t increment);
pid_t getpid();