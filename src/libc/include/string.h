#pragma once

#ifndef NULL
#define NULL ((void*)0)
#endif

typedef unsigned int size_t;

void* memcpy(void* dst, const void* src, size_t n);
void* memset(void* dst, int c, size_t n);
int memcmp(const void* a, const void* b, size_t n);
void* memmove(void* dst, const void* src, size_t length);

size_t strlen(const char* s);
int strcmp(const char* a, const char* b);
char* strcpy(char* dst, const char* src);
char* strncpy(char* dst, const char* src, size_t n);

char* strerror(int errnum);