void exit(int r)
{
    asm("int 0xff" : 
        : "a" (0), "b" (r));
    while(1);
}

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

static char encoding_table[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
    'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
    'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
    'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
    'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
    'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
    'w', 'x', 'y', 'z', '0', '1', '2', '3',
    '4', '5', '6', '7', '8', '9', '+', '/'};
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

char b64l_buffer[7];

char* b64l(long value)
{
    uint8_t length = 0;
    long offset = 60;   // biggest multiple of 6 less than 64
    bool first0 = true;
    for (uint8_t i = 0; i < 6; i++)
    {
        char c = encoding_table[(value >> offset) % 64];
        first0 &= c == 0;
        if (!first0)
        {
            b64l_buffer[length] = c;
            length++;
        }
        offset -= 6;
    }
    b64l_buffer[length] = 0;
    return b64l_buffer;
}