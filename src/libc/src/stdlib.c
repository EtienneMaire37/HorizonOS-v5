void exit(int r)
{
    asm("int 0xff" : 
        : "a" (0), "b" (r));
    while(1);
}