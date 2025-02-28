int main();
void exit();

void _start()
{
    errno = 0;
    create_b64_decoding_table();
    memset(atexit_stack, NULL, 32);
    atexit_stack_length = 0;
    int return_value = main();
    exit(return_value);
}