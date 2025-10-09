#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <limits.h>

#include <horizonos.h>

#include "../../libc/src/misc.h"

const char* kb_layouts[] = {"us_qwerty", "fr_azerty"};

int main(int argc, char** argv)
{
    printf("--- Start of HorizonOS configuration ---\n\n");

    printf("Please enter your preferred keyboard layout:\n");
    for (uint8_t i = 0; i < sizeof(kb_layouts) / sizeof(char*); i++)
        printf("%u: %s    ", i + 1, kb_layouts[i]);
    putchar('\n');
    uint8_t kb_layout_choice = 0;
    while (!(kb_layout_choice >= 1 && kb_layout_choice <= 2))
    {
        printf("->");
        fflush(stdout);
        char kb_layout_choice_str[2] = { 0 };
        read(STDIN_FILENO, &kb_layout_choice_str[0], 2);
        if (kb_layout_choice_str[1] != '\n')
            kb_layout_choice = 0;
        else
        {
            kb_layout_choice_str[1] = 0;
            kb_layout_choice = atoi(kb_layout_choice_str);
        }
        flush_stdin();
    }

    if (set_kb_layout(kb_layout_choice))
        printf("Successfully set keyboard layout to : %s\n", kb_layouts[kb_layout_choice - 1]);
    else
        printf("Error : Defaulting to the us_qwerty keyboard layout\n");

    putchar('\n');

    execvp("shell", (char*[]){"shell"});
}