#pragma once

char hex[16] = "0123456789abcdef";
char HEX[16] = "0123456789ABCDEF";

#include "../../libc/include/stdio.h"

#define kFILE       FILE
#define kstdin      stdin
#define kstdout     stdout
#define kstderr     stderr
#define klog        ((FILE*)3)

kFILE* current_stream, *current_interrupt_stream;

void kputchar(char c);
void kputs(char* str);

void kfprintf(kFILE* file, char* fmt, ...);

#define kprintf(...) kfprintf(kstdout, __VA_ARGS__)