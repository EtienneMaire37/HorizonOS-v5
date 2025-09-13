#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#include <horizonos.h>

const char* kb_layouts[] = {"us_qwerty", "fr_azerty"};

int main()
{
    printf("--- Start of HorizonOS configuration ---\n\n");

    // volatile int a = 1 / 0;

    // uint8_t* data = (uint8_t*)malloc(5000000);
    // srand(time(NULL));
    // for (int i = 0; i < 5000000; i++)
    //     data[i] = rand() & 0xff;

    // for (int i = 0; i < 10; i++)
    //     printf("0x%x\n", data[i + 200000]);

    // free(data);

    // uint8_t data[50000];
    // for (int i = 0; i < 50000; i++)
    //     data[i] = rand() & 0xff;

    // for (int i = 0; i < 10; i++)
    //     printf("0x%x\n", data[i + 20000]);

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
        if (kb_layout_choice_str[1] != EOF && kb_layout_choice_str[1] != '\n')
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

    // volatile uint8_t data[0x8000] = { 0 };

    while (true)
    {
        // printf("%s\n", &data[0]);
        printf("$ ");
        fflush(stdout);
        char ch = 0;
        read(STDIN_FILENO, &ch, 1);
        flush_stdin();
    }
    return 0;
}