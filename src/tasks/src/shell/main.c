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
#include <termios.h>

#include "../../libc/src/misc.h"

const char* kb_layouts[] = {"us_qwerty", "fr_azerty"};

// void int_handler()
// {
//     putchar('\n');
// }

extern char** environ;

struct termios oldt, newt;

void set_raw_mode(bool enable) 
{
    if (enable) 
    {
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO);   // * Disable canonical mode and echoing
        newt.c_cc[VMIN] = 1;
        newt.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    } 
    else
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
}

void disable_raw_mode_and_exit()
{
    set_raw_mode(false);
    exit(0);
}

int main(int argc, char** argv)
{
    // struct sigaction sa;
    // sa.sa_handler = int_handler;
    // sigemptyset(&(sa.sa_mask));
    // sigaddset(&(sa.sa_mask), SIGINT);
    // sigaction(SIGINT, &sa, NULL);

    set_raw_mode(true);

    setenv("?", "0", true);

    char cwd[PATH_MAX];

    while (true)
    {
        getcwd(cwd, sizeof(cwd));
        printf("\x1b[36m%s\x1b[0m$ ", cwd);
        fflush(stdout);
        char data[BUFSIZ] = {0};
        char byte;
        int write_position = 0;
        while (read(STDIN_FILENO, &byte, 1) > 0)
        {
            if (byte == '\n' || byte == newt.c_cc[VEOF] || write_position > BUFSIZ - 1)
                break;
            else
            {
                switch (byte)
                {
                case '\b':
                    if (write_position > 0)
                    {
                        write_position--;
                        printf("\b \b");
                    }
                    break;
                case '\x1b':    // * escape sequence
                {
                    char next_bytes[8];
                    read(STDIN_FILENO, &next_bytes[0], 1);
                    read(STDIN_FILENO, &next_bytes[1], 1);
                    int byte_count = 1;
                    while (next_bytes[byte_count] >= '0' && next_bytes[byte_count] <= '9' && byte_count < 7)
                        read(STDIN_FILENO, &next_bytes[++byte_count], 1);
                    if (next_bytes[0] == '[')
                    {
                        switch (next_bytes[1])
                        {
                        case 'A':   // * Up arrow
                            
                            break;
                        default:
                            ;
                        }
                    }
                    break;
                }
                default:
                    data[write_position++] = byte;
                    putchar(byte);
                }
            }
            fflush(stdout);
        }
        data[write_position] = 0;
        putchar('\n');

        char data_processed[BUFSIZ] = {0};
        memcpy(data_processed, data, BUFSIZ);
        bool string = false;
        for (int i = 0; i < BUFSIZ - 1; i++)
        {
            if (data_processed[i] == ' ' && !string) data_processed[i] = 0;
            if (data_processed[i] == '\"') 
            {
                string ^= true;
                data_processed[i] = 0;
            }
        }
        int bytes_left = write_position - 1;
        char* first_arg = find_first_arg(data_processed, &bytes_left);
        if (!first_arg) goto cmd;
        if (*first_arg == 0) goto cmd;
        if (strcmp(first_arg, "exit") == 0)
        {
            disable_raw_mode_and_exit();
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
    disable_raw_mode_and_exit();
}