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

    return 0;
}