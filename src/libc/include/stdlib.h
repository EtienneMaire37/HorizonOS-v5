#pragma once

#ifndef NULL
#define NULL ((void*)0)
#endif

#define RAND_MAX 32767
#define EXIT_FAILURE (-1)
#define EXIT_SUCCESS 0
#define MB_CUR_MAX   1  // Only ASCII is supported

typedef unsigned int size_t;
typedef char wchar_t;

void exit(int r);
int rand();
void srand(unsigned int seed);

long a64l(const char *s);
char* l64a(long value);
int abs(int n);
int atexit(void (*function)(void));
int isdigit(int c);
int isspace(int c);
int isalpha(int c);
int tolower(int c);
int toupper(int c);
int atoi(const char* str);
void abort(void);
long strtol(const char* nptr, char** endptr, int base);
int atoi(const char* str);

#include "../src/liballoc.h"