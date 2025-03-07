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
    // printf("PI: %lf\n", M_PIl);

    // for (double i = 0.; i < 1000.; i += 0.05)
    // {
    //     printf("log(%f) = %f\n", i, log(i));
    // }

    printf("log(%f) = %f\n", 10000000., log(10000000.));

    return 0;
}