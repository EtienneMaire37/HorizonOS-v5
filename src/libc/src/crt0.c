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

    FILE* FILE_create()
    {
        FILE* f = (FILE*)malloc(sizeof(FILE));
        if (f == NULL) return NULL;
        f->input_buffer = (uint8_t*)malloc(BUFSIZ);
        f->output_buffer = (uint8_t*)malloc(BUFSIZ);
        return f;
    }

    stdin = FILE_create();
    if (stdin == NULL) exit(EXIT_FAILURE);
    if (stdin->input_buffer == NULL || stdin->output_buffer == NULL) exit(EXIT_FAILURE);
    stdin->fd = STDIN_FILENO;
    stdout = FILE_create();
    if (stdout == NULL) exit(EXIT_FAILURE);
    if (stdout->input_buffer == NULL || stdout->output_buffer == NULL) exit(EXIT_FAILURE);
    stdout->fd = STDOUT_FILENO;
    stderr = FILE_create();
    if (stderr == NULL) exit(EXIT_FAILURE);
    if (stderr->input_buffer == NULL || stderr->output_buffer == NULL) exit(EXIT_FAILURE);
    stderr->fd = STDERR_FILENO;

    create_b64_decoding_table();

    memset(atexit_stack, 0, 32);
    atexit_stack_length = 0;

    exit(main());
    while(true); // ^ It would GPF <<- { asm volatile ("hlt"); };
}