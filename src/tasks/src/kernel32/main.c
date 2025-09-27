// #define _POSIX_C_SOURCE 200809L

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
// #include <signal.h>
#include <limits.h>

#include <horizonos.h>

const char* kb_layouts[] = {"us_qwerty", "fr_azerty"};

char* find_next_contiguous_string(char* str, int* bytes_left)
{
    if (!str) return NULL;
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

// void int_handler()
// {
//     putchar('\n');
// }

int main()
{
    // struct sigaction sa;
    // sa.sa_handler = int_handler;
    // sigemptyset(&(sa.sa_mask));
    // sigaddset(&(sa.sa_mask), SIGINT);
    // sigaction(SIGINT, &sa, NULL);

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

    char cwd[PATH_MAX];

    while (true)
    {
        getcwd(cwd, sizeof(cwd));
        printf("%s$ ", cwd);
        fflush(stdout);
        char data[4096] = {0};
        int ret = read(STDIN_FILENO, &data, 4096);
        flush_stdin();

        if (ret > 0)
        {
            data[ret - 1] = 0;
            char data_processed[4096] = {0};
            memcpy(data_processed, data, 4096);
            bool string = false;
            for (int i = 0; i < 4095; i++)
            {
                if (data_processed[i] == ' ' && !string) data_processed[i] = 0;
                if (data_processed[i] == '\"') 
                {
                    string ^= true;
                    data_processed[i] = 0;
                }
            }
            int bytes_left = ret - 1;
            char* first_arg = data_processed[0] ? data_processed : find_next_contiguous_string(data_processed, &bytes_left);
            if (!first_arg) goto cmd;
            if (*first_arg == 0) goto cmd;
            if (strcmp(first_arg, "exit") == 0)
            {
                exit(EXIT_SUCCESS);
            }
            else if (strcmp(first_arg, "cd") == 0)
            {
                first_arg = find_next_contiguous_string(first_arg, &bytes_left);
                char* arg = find_next_contiguous_string(first_arg, &bytes_left);
                if (first_arg == NULL)
                {
                    fprintf(stderr, "cd: not enough arguments\n");
                    continue;
                }
                if (arg != NULL)
                {
                    fprintf(stderr, "cd: too many arguments\n");
                    continue;
                }
                if (chdir(first_arg) != 0)
                {
                    switch(errno)
                    {
                    case ENOENT:
                        fprintf(stderr, "cd: no such file or directory\n");
                        break;
                    case ENOTDIR:
                        fprintf(stderr, "cd: not a directory\n");
                        break;
                    default:
                        fprintf(stderr, "cd: couldn't access directory\n");
                    }
                }
            }
            else 
            {
            cmd:
                system(data);
            }
        }
    }
    return 0;
}

// #include <stdio.h>
// #include <limits.h>
// #include <unistd.h>
// #include <stdlib.h>

// int main()
// {
//     printf("ABCDEF: %s\n", getenv("ABCDEF"));
//     printf("PATH: %s\n", getenv("PATH"));
// }