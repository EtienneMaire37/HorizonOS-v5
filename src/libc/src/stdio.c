int putchar(int c)
{
    asm("int 0xff" : : "a" ((char)c));
}