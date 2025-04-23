#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

int main()
{    
    printf("fork() : %ld\n", fork());
    printf("fork() : %ld\n", fork());
    printf("fork() : %ld\n", fork());
    printf("fork() : %ld\n", fork());
    printf("pid : %lu\n", getpid());

    return 0;
}