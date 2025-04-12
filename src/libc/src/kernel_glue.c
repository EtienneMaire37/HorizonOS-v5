#pragma once

int fputc(int c, FILE* stream)
{
    asm volatile("int 0xff" : 
        : "a" (1), "b" ((char)c), "c" (stream));
    return c;
}

void exit(int r)
{
    for (uint8_t i = 0; i < atexit_stack_length; i++)
        if (atexit_stack[atexit_stack_length - i - 1] != NULL)
            atexit_stack[atexit_stack_length - i - 1]();

    asm volatile("int 0xff" : 
        : "a" (0), "b" (r));
    while(1);
}

time_t time(time_t* t)
{
    time_t now = 0;
    asm volatile("int 0xff" : "=a" (now)
        : "a" (2));
    if (t) *t = now;
    return now;
}

pid_t getpid()
{
    uint32_t hi, lo;
    asm volatile("int 0xff" : "=a" (hi), "=b" (lo)
        : "a" (3));
    return ((pid_t)hi << 32) | lo;
}

pid_t fork()
{
    uint32_t hi, lo;
    asm volatile("int 0xff" : "=a" (hi), "=b" (lo)
        : "a" (4));
    return ((pid_t)hi << 32) | lo;
}

// void* sbrk(intptr_t increment)
// {
//     intptr_t incremented;
//     asm volatile("int 0xff" : "=a" (incremented)
//         : "a" (5), "b" (increment));
//     break_point += incremented;
//     return (void*)break_point;
// }