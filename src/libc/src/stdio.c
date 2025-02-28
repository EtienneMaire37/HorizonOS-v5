int putchar(int c)
{
    asm("int 0xff" : 
        : "a" (1), "b" ((char)c));
    return c;
}

int puts(const char* s)
{
    while(*s)
        putchar(*s++);
    return 0;
}

FILE* fopen(const char* path, const char* mode)
{
    return NULL;
}

int fclose(FILE* stream)
{
    return 0;
}