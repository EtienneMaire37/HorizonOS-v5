#pragma once

#ifndef NULL
#define NULL ((void*)0)
#endif

typedef unsigned int size_t;

void* memset(void* ptr, int value, size_t num);
void* memcpy(void* destination, const void* source, size_t num);