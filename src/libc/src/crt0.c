int main();
extern void* _break_address;

void _main()
{
    // dprintf(STDERR_FILENO, "_main\n");

    const int default_environ = 1;
    environ = malloc((default_environ + 1) * sizeof(char*));
    environ[0] = "PATH=";
    environ[default_environ] = NULL;

    memset(atexit_stack, 0, 32);
    atexit_stack_length = 0;

    errno = 0;
    heap_size = 0;
    break_address = (uint32_t)&_break_address;
    break_address = ((break_address + 4095) / 4096) * 4096;
    heap_address = break_address;
    alloc_break_address = break_address;
    malloc_bitmap_init();

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

    exit(main());
    while(true);
}