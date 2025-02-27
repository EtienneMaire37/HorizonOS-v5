void exit(int r)
{
    asm("int 0xff" : : "a"(0x80 | r));
    while(1);
}