#pragma once

#ifndef NULL
#define NULL ((void*)0)
#endif

typedef unsigned int size_t;

void* memset(void* ptr, int value, size_t num);
void* memcpy(void* destination, const void* source, size_t num);
int memcmp(const void* str1, const void* str2, size_t n);
int strcmp(const char* str1, const char* str2);
size_t strlen(const char* str);