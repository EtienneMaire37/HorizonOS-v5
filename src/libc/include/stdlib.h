#pragma once

#ifndef NULL
#define NULL ((void*)0)
#endif

#define RAND_MAX 32767
#define EXIT_FAILURE (-1)
#define EXIT_SUCCESS 0

void exit(int r);
int rand();
void srand(unsigned int seed);