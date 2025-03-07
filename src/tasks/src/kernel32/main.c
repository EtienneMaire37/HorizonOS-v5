#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <math.h>

int main()
{
    printf("isfinite : \n");
    printf("0.: %u\n", isfinite(0.));
    printf("1. / 0.: %u\n", isfinite(1. / 0.));
    printf("PI: %f\n", M_PI);

    printf("ln(%f) = %f\n", 10000000., log(10000000.));
    printf("ln(%f) = %f\n", 10000., log(10000.));
    printf("ln(%f) = %f\n", 0., log(0.));
    printf("ln(%f) = %f\n", 1., log(1.));
    printf("ln(%f) = %f\n", M_E, log(M_E));
    printf("ln(%f) = %f\n", 3.5, log(3.5));
    printf("sin(%f) = %f\n", 15., sin(15.));
    printf("cos(%f) = %f\n", .7, cos(.7));

    return 0;
}