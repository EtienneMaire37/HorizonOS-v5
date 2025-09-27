int main();
extern void* _break_address;

extern uint32_t kernel_data;

char* default_environ[] = {NULL};

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
    
    // printf("%s\n\n", (char*)kernel_data);

    exit(main());
    while(true);
}