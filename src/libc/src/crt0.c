extern void* _break_address;
extern void call_main_exit(int argc, char** argv);

extern uint32_t kernel_data;

char* default_environ[] = {NULL};

static char* find_next_contiguous_string(char* str, int* bytes_left)
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

void _main()
{
    // dprintf(STDERR_FILENO, "_main\n");

    memset(atexit_stack, 0, 32);
    atexit_stack_length = 0;

    errno = 0;
    heap_size = 0;
    break_address = (uint32_t)&_break_address;
    break_address = ((break_address + 4095) / 4096) * 4096;
    heap_address = break_address;
    alloc_break_address = break_address;
    malloc_bitmap_init();

    const int num_environ = 1;
    environ = malloc((num_environ + 1) * sizeof(char*));
    if (!environ) environ = &default_environ[0];
    else
    {
        environ[0] = "PATH=";
        environ[num_environ] = NULL;
    }

    // dprintf(STDERR_FILENO, "Initializing buffered streams...\n");

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

    // dprintf(STDERR_FILENO, "Calling main...\n");

    startup_data_struct_t* data = (startup_data_struct_t*)kernel_data;
    
    // printf("%s\n\n", data->cmd_line);

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
    argv = malloc(argc * sizeof(char*));
    bytes_left = sizeof(data->cmd_line) - 1;
    arg = data->cmd_line;
    if (*arg)
        argv[i++] = arg;
    while (arg = find_next_contiguous_string(arg, &bytes_left))
        argv[i++] = arg;

    memcpy(cwd, data->pwd, PATH_MAX);

    call_main_exit(argc, argv);
    while(true);
}