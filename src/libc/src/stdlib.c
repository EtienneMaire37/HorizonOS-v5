void exit(int r)
{
    asm("int 0xff" : 
        : "a" (0), "b" (r));
    while(1);
}

uint32_t rand_next = 1;

int rand()
{
    rand_next = rand_next * 1103515245 + 12345;
    return (int)((rand_next / 65536) % (RAND_MAX + 1));
}

void srand(unsigned int seed)
{
    rand_next = seed;
}