int putchar(int c)
{
    asm("int 0xff" : 
        : "a" (1), "b" ((char)c));
    return c;
}