#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>

#include <horizonos.h>

void flush_stdin()
{
    // ...
}

int main()
{
    printf("--- Start of HorizonOS configuration ---\n\n");
    printf("Please enter your preferred keyboard layout:\n");
    printf("1: us_qwerty      2: fr_azerty\n");
    printf("->");
    char kb_layout_choice_str[2] = { 0 };
    read(STDIN_FILENO, &kb_layout_choice_str[0], 1);
    uint8_t kb_layout_choice = atoi(kb_layout_choice_str);
    flush_stdin();
    return 0;
}