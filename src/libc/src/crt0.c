int main();
void exit();
extern void* _break_point;

void _start()
{
    errno = 0;
    heap_size = 0;
    break_point = (uint32_t)&_break_point;
    // printf("Break point : 0x%x\n\n", break_point);
    create_b64_decoding_table();
    memset(atexit_stack, 0, 32);
    atexit_stack_length = 0;
    int return_value = main();
    exit(return_value);
}