#pragma once

#include "stdarg.h"

#ifndef NULL
#define NULL ((void*)0)
#endif

typedef unsigned int size_t;

#define EOF (-1)

#define SEEK_CUR 0
#define SEEK_END 1
#define SEEK_SET 2

#define BUFSIZ 4096

typedef unsigned int fpos_t;

#define _IOFBF 0
#define _IOLBF 1
#define _IONBF 2

#define FILENAME_MAX    255
#define FOPEN_MAX       16
#define TMP_MAX         65536

typedef struct FILE FILE;
struct FILE
{
    int fd;
    unsigned char* input_buffer;
    unsigned char* output_buffer;
};

// FILE* stdin;
// FILE* stdout;
// FILE* stderr;

// FILE* fopen(const char* path, const char* mode);
// int fclose(FILE* stream);

// int fputc(int c, FILE* stream);
// #define putc fputc
int putchar(int c);
// int fputs(const char* s, FILE* stream);
int puts(const char* s);

// int fprintf(FILE* stream, const char* format, ...);
// int vfprintf(FILE* stream, const char* format, va_list args);
int printf(const char* format, ...);
int vprintf(const char* format, va_list args);
int dprintf(int fd, const char *format, ...);
int vdprintf(int fd, const char *format, va_list args);
int sprintf(char* buffer, const char* format, ...);
int snprintf(char* buffer, size_t bufsz, const char* format, ...);
int vsprintf(char* buffer, const char* format, va_list args);
int vsnprintf(char* buffer, size_t bufsz, const char* format, va_list args);

int getchar();