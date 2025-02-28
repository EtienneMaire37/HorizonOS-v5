int main();
void exit();

void _start()
{
    create_b64_decoding_table();
    exit(main());
}