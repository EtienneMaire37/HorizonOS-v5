extern void* _break_address;
extern void call_main_exit(int argc, char** argv);

extern uint32_t kernel_data;

void _main()
{
#include "misc.h"
    memset(atexit_stack, 0, 32);
    atexit_stack_length = 0;

    errno = 0;
    heap_size = 0;
    break_address = (uint32_t)&_break_address;
    break_address = ((break_address + 4095) / 4096) * 4096;
    heap_address = break_address;
    alloc_break_address = break_address;
    malloc_bitmap_init();

    startup_data_struct_t* data = (startup_data_struct_t*)kernel_data;

    environ = data->environ;
    int environ_num = 0;
    while (environ[environ_num])
        environ_num++;
    char** _environ = malloc((environ_num + 1) * sizeof(char*));
    if (_environ)
    {
        for (int i = 0; i < environ_num; i++)
        {
            _environ[i] = malloc((strlen(environ[i]) + 1));
            if (!_environ[i])
                abort();
            strcpy(_environ[i], environ[i]);
        }
        num_environ = environ_num;
        environ = _environ;
    }
    else
    {
        environ = NULL; // abort();
        num_environ = 0;
    }

    stdin = FILE_create();
    if (stdin == NULL) exit(EXIT_FAILURE);
    stdin->fd = STDIN_FILENO;
    stdin->flags = FILE_FLAGS_READ;

    stdout = FILE_create();
    if (stdout == NULL) exit(EXIT_FAILURE);
    stdout->fd = STDOUT_FILENO;
    stdout->flags = FILE_FLAGS_WRITE | FILE_FLAGS_LBF;

    stderr = FILE_create();
    if (stderr == NULL) exit(EXIT_FAILURE);
    stderr->fd = STDERR_FILENO;
    stderr->flags = FILE_FLAGS_WRITE | FILE_FLAGS_NBF;

    create_b64_decoding_table();

    bool string = false;
    for (int i = 0; i < 4096; i++)
    {
        if (data->cmd_line[i] == ' ' && !string) data->cmd_line[i] = 0;
        if (data->cmd_line[i] == '\"') 
        {
            string ^= true;
            data->cmd_line[i] = 0;
        }
    }

    int argc = 0;
    char** argv = NULL;
    int bytes_left = sizeof(data->cmd_line) - 1;
    char* arg = data->cmd_line;
    if (*arg)
        argc++;
    while (arg = find_next_contiguous_string(arg, &bytes_left))
        argc++;

    int i = 0;
    argv = malloc((1 + argc) * sizeof(char*));
    if (argv)
    {
        bytes_left = sizeof(data->cmd_line) - 1;
        arg = data->cmd_line;
        if (*arg)
            argv[i++] = arg;
        while (arg = find_next_contiguous_string(arg, &bytes_left))
            argv[i++] = arg;

        argv[i] = 0;
    }

    // printf("argc : %d\n", argc);

    memcpy(cwd, data->pwd, PATH_MAX);
    realpath(cwd, cwd);

    call_main_exit(argc, argv);
    while(true);
}