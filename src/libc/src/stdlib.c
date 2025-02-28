uint32_t rand_next = 1;

int rand()
{
    rand_next = rand_next * 1103515245 + 12345;
    return (int)((rand_next / 65536) % (RAND_MAX + 1));
}

void srand(unsigned int seed)
{
    rand_next = seed;
}

static char* encoding_table = "./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
static char decoding_table[256];

void create_b64_decoding_table()
{
    memset(decoding_table, 0xff, 256);
    for (int i = 0; i < 64; i++)
        decoding_table[(unsigned char)encoding_table[i]] = i;
}

long a64l(const char* s)
{
    long result = 0;
    unsigned long magnitude = 1;
    uint8_t i = 0;
    while (*s && i < 6)
    {
        uint8_t digit = decoding_table[*s];
        if (digit == 0xff)
            break;
        result += digit * magnitude;
        s++;
        i++;
        magnitude *= 64;
    }
    return result;
}

char l64a_buffer[7];

char* l64a(long value)
{
    if (value < 0)
        return NULL;
    uint8_t length = 0;
    for (uint8_t i = 0; i < 6; i++)
    {
        if (value == 0)
            break;
        l64a_buffer[i] = encoding_table[value & 0x3f];
        value >>= 6;
        length++;
    }
    l64a_buffer[length] = 0;
    return l64a_buffer;
}

int abs(int n)
{
    return n < 0 ? -n : n;
}

void (*atexit_stack[32])();
uint8_t atexit_stack_length;

int atexit(void (*function)(void))
{
    if (atexit_stack_length >= 32)
        return 1;
    atexit_stack[atexit_stack_length++] = function;
    return 0;
}

void exit(int r)
{
    for (uint8_t i = 0; i < atexit_stack_length; i++)
        if (atexit_stack[atexit_stack_length - i - 1] != NULL)
            atexit_stack[atexit_stack_length - i - 1]();

    asm("int 0xff" : 
        : "a" (0), "b" (r));
    while(1);
}