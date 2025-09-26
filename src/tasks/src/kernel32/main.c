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

#include <horizonos.h>

const char* kb_layouts[] = {"us_qwerty", "fr_azerty"};

char* find_next_contiguous_string(char* str, int* bytes_left)
{
    if (!bytes_left) return NULL;
    while (*str && (*bytes_left) > 0)
    {
        str++;
        (*bytes_left)--;
    }
    while (!(*str) && (*bytes_left) > 0)
    {
        str++;
        (*bytes_left)--;
    }
    if ((*bytes_left) <= 0) return NULL;
    return str;
}

int main()
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

    // volatile uint8_t data[0x8000] = { 0 };

    while (true)
    {
        // printf("%s\n", &data[0]);
        printf("$ ");
        fflush(stdout);
        char data[4096] = {0};
        int ret = read(STDIN_FILENO, &data, 4096);
        flush_stdin();

        if (ret > 0)
        {
            data[ret - 1] = 0;
            bool string = false;
            for (int i = 0; i < 4095; i++)
            {
                if (data[i] == ' ' && !string) data[i] = 0;
                if (data[i] == '\"') 
                {
                    string ^= true;
                    data[i] = 0;
                }
            }
            int bytes_left = ret - 1;
            char* first_arg = data[0] ? data : find_next_contiguous_string(data, &bytes_left);
            if (!first_arg) continue;
            if (*first_arg == 0) continue;
            if (strcmp(first_arg, "exit") == 0)
            {
                exit(EXIT_SUCCESS);
            }
            else if (strcmp(first_arg, "echo") == 0)
            {
                char* arg = find_next_contiguous_string(first_arg, &bytes_left);
                while (arg) 
                {
                    printf("%s ", arg);
                    arg = find_next_contiguous_string(arg, &bytes_left);
                }
                putchar('\n');
            }
            else
            {
                printf("%s: command not found\n", first_arg);
            }
        }
    }
    return 0;
}
