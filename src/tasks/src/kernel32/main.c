#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <math.h>

int main()
{
    printf("PIl: %lf\n", M_PIl);
    printf("exp(%f) = %f\n", 32.57, exp(32.57));
    // printf("ln(%f) = %f\n", 10000000., log(10000000.));
    printf("exp(%f) = %f\n", -1.5, exp(-1.5));
    printf("ln(%f) = %f\n", 10000., log(10000.));
    printf("ln(%f) = %f\n", 0., log(0.));
    printf("ln(%f) = %f\n", 1., log(1.));
    printf("ln(%f) = %f\n", M_E, log(M_E));
    printf("ln(%f) = %f\n", 3.5, log(3.5));
    printf("sin(%f) = %f\n", 15., sin(15.));
    printf("cos(%f) = %f\n", .7, cos(.7));
    printf("2^%f = %f\n", 13., exp2(13.));
    printf("2^%f = %f\n", 15.5, exp2(15.5));
    printf("sqrt(%f) = %f\n", 128., sqrt(128.));
    printf("sqrt(%f) = %f\n", .35, sqrt(.35));
    printf("sqrt(%f) = %f\n", 2., sqrt(2.));
    printf("sqrt(%f) = %f\n", 1526.3, sqrt(1526.3));
    printf("%f\n", 133.16789134678891200042345);
    
    printf("fork() : %ld\n", fork());
    printf("pid : %lu\n", getpid());

    // while(true);
    return 0;
}