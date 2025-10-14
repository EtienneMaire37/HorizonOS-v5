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
#include <sys/wait.h>

#include <horizonos.h>

#include "../../libc/src/misc.h"

const char* kb_layouts[] = {"us_qwerty", "fr_azerty"};

// void int_handler()
// {
//     putchar('\n');
// }

extern char** environ;

int main(int argc, char** argv)
{
    // struct sigaction sa;
    // sa.sa_handler = int_handler;
    // sigemptyset(&(sa.sa_mask));
    // sigaddset(&(sa.sa_mask), SIGINT);
    // sigaction(SIGINT, &sa, NULL);

    setenv("?", "0", true);

    char cwd[PATH_MAX];

    while (true)
    {
        getcwd(cwd, sizeof(cwd));
        printf("\x1b[36m%s\x1b[0m$ ", cwd);
        fflush(stdout);
        char data[4096] = {0};
        int ret = read(STDIN_FILENO, &data, 4096);

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
            char* first_arg = find_first_arg(data_processed, &bytes_left);
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
                    char buf[PATH_MAX + 4];
                    snprintf(buf, PATH_MAX + 4, "cd: %s", first_arg);
                    perror(buf);
                }
            }
            else 
            {
            cmd:
                int status = system(data);
                int exit_code = -1;
                if (status != -1) 
                {
                    if (WIFEXITED(status))
                        exit_code = WEXITSTATUS(status);
                    // else
                    //     exit_code = 128 + WTERMSIG(status);
                }
                char buf[16];
                snprintf(buf, sizeof(buf), "%d", exit_code);
                setenv("?", buf, true);
                // printf("%d\n", exit_code);
            }
        }
    }
    return 0;
}