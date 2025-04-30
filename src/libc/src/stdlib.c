#include <limits.h>

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
        uint8_t digit = decoding_table[(uint8_t)*s];
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

int isdigit(int c)
{
    return (c >= '0' && c <= '9');
}

int isspace(int c)
{
    return (c == ' ' || c == '\t' || c == '\n' || 
            c == '\v' || c == '\f' || c == '\r');
}

int isalpha(int c)
{
    return ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'));
}

int tolower(int c)
{
    if (c >= 'A' && c <= 'Z')
        return c + ('a' - 'A');
    return c;
}

int toupper(int c)
{
    if (c >= 'a' && c <= 'z')
        return c - ('a' - 'A');
    return c;
}

long strtol(const char* nptr, char** endptr, int base) 
{
    const char *s = nptr;
    long result = 0;
    int neg = 0;
    int overflow = 0;
    int any_digits = 0;
    long cutoff;
    int cutlim;

    while (isspace((unsigned char)*s))
        s++;

    if (*s == '-') 
    {
        neg = 1;
        s++;
    } 
    else if (*s == '+')
        s++;

    if (base == 0) 
    {
        if (*s == '0') 
        {
            if (s[1] == 'x' || s[1] == 'X') 
            {
                base = 16;
                s += 2;
            } 
            else 
            {
                base = 8;
                s++;
            }
        } 
        else 
        {
            base = 10;
        }
    } 
    else if (base == 16) 
    {
        if (*s == '0' && (s[1] == 'x' || s[1] == 'X'))
            s += 2;
    }

    if (base < 2 || base > 36) 
    {
        if (endptr != NULL) 
            *endptr = (char *)nptr;
        errno = EINVAL;
        return 0;
    }

    cutoff = neg ? LONG_MIN / base : LONG_MAX / base;
    cutlim = neg ? -(LONG_MIN % base) : LONG_MAX % base;

    while(true)
    {
        int c = (unsigned char)*s;
        int digit;

        if (isdigit(c)) 
            digit = c - '0';
        else if (isalpha(c)) 
            digit = tolower(c) - 'a' + 10;
        else
            break; 

        if (digit >= base)
            break;

        any_digits = 1;

        if (neg) 
        {
            if (result < cutoff || (result == cutoff && digit > cutlim)) 
            {
                overflow = 1;
                break;
            }
            result = result * base - digit;
        } 
        else 
        {
            if (result > cutoff || (result == cutoff && digit > cutlim)) 
            {
                overflow = 1;
                break;
            }
            result = result * base + digit;
        }
        s++;
    }

    if (!any_digits) 
    {
        if (endptr != NULL) 
            *endptr = (char *)nptr;
        errno = EINVAL;
        return 0;
    }

    if (overflow) 
    {
        errno = ERANGE;
        result = neg ? LONG_MIN : LONG_MAX;
    } 
    else if (neg) 
        result = -result;

    if (endptr != NULL) 
        *endptr = (char *)s;

    return result;
}

int atoi(const char* str)
{
    return (int)strtol(str, (char**)NULL, 10);
}

void abort()
{
    exit(EXIT_FAILURE);
}