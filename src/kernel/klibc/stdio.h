#pragma once

char hex[16] = "0123456789abcdef";
char HEX[16] = "0123456789ABCDEF";

typedef enum kstream
{
    STDIN_STREAM,
    STDOUT_STREAM,
    STDERR_STREAM,
    LOG_STREAM,
    FILE_STREAM
} kstream_t;

typedef struct kFILE
{
    kstream_t stream;
} kFILE;

kFILE _kstdin;
kFILE _kstdout;
kFILE _kstderr;
kFILE _klog;

kFILE* kstdin = &_kstdin;
kFILE* kstdout = &_kstdout;
kFILE* kstderr = &_kstderr;
kFILE* klog = &_klog;

kFILE* current_stream, *current_interrupt_stream;

void kputchar(char c);
void kputs(char* str);

void kprintf_d(int64_t val);
void kprintf_u(uint64_t val);
void kprintf_x(uint64_t val, uint8_t padding);
void kprintf_X(uint64_t val, uint8_t padding);
void kfprintf(kFILE* file, char* fmt, ...);
void kgets(char* str);

#define kprintf(...) kfprintf(kstdout, __VA_ARGS__)