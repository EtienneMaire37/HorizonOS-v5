int main();
extern void* _break_address;

void _main()
{
    errno = 0;
    heap_size = 0;
    break_address = (uint32_t)&_break_address;
    break_address = ((break_address + 4095) / 4096) * 4096;
    heap_address = break_address;
    alloc_break_address = break_address;
    malloc_bitmap_init();
    // // printf("Break address : 0x%x\n\n", break_address);

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

    memset(atexit_stack, 0, 32);
    atexit_stack_length = 0;

    exit(main());
    while(true); // ^ It would GPF <<- { asm volatile ("hlt"); };
}