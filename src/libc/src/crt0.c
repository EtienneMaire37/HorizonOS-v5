int main();
void exit();
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
    // printf("Break address : 0x%x\n\n", break_address);
    create_b64_decoding_table();
    memset(atexit_stack, 0, 32);
    atexit_stack_length = 0;
    int return_value = main();
    exit(return_value);
    while(true) { asm volatile ("hlt"); };
}