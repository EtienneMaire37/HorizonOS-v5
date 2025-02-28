#pragma once

#ifndef NULL
#define NULL ((void*)0)
#endif

#define EOF (-1)

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#define BUFSIZ 1024

typedef unsigned int fpos_t;

typedef struct FILE FILE;
struct FILE
{
    char path[256];
    fpos_t pos;
    unsigned char drive;
    // uint8_t mode;
};

#define stdin ((FILE*)0)
#define stdout ((FILE*)1)
#define stderr ((FILE*)2)

FILE* fopen(const char* path, const char* mode);
int fclose(FILE* stream);

int fputc(int c, FILE* stream);
// int putc(int c, FILE* stream);
#define putc fputc
int putchar(int c);
int fputs(const char* s, FILE* stream);
int puts(const char* s);
int fprintf(FILE* stream, const char* format, ...);
#define printf(...) fprintf(stdout, __VA_ARGS__);
