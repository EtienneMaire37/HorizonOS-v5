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
    // memset(decoding_table, 0xff, 256);
    for (int i = 0; i < 256; i++)
        decoding_table[i] = 0xff;
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

#ifdef BUILDING_KERNEL
#define abort() (printf("Kernel aborted. (func: \"%s\"; line: %d; file: \"%s\")\n", __CURRENT_FUNC__, __LINE__, __FILE__), halt())
#else
void abort()
{
    exit(EXIT_FAILURE);
}
#endif

void malloc_bitmap_init();
bool malloc_bitmap_get_page(uint32_t page);
void malloc_bitmap_set_page(uint32_t page, bool state);

char* getenv(const char* name)
{
    if (name == NULL) return NULL;
    if (environ == NULL) return NULL;

    int i = 0;
    while (environ[i])
    {
        int j = 0;
        while (environ[i][j] == name[j] && name[j] != 0 && environ[i][j] != 0 && environ[i][j] != '=')
            j++;
        if (environ[i][j] == '=')
            return &environ[i][j + 1];
        i++;
    }

    return NULL;
}

int system(const char* command)
{
#include "misc.h"
    if (!command) return 1;

    size_t bytes = strlen(command) + 1;
    char* cmd_data = malloc(bytes);
    if (!cmd_data) 
    {
        errno = EAGAIN;
        return -1;
    }
    int return_value = 0;
    strncpy(cmd_data, command, bytes);

    if (bytes <= 1)
    {
        return_value = 1;
        goto do_return;
    }

    size_t characters = bytes - 1;
    bool string = false;
    for (int i = 0; i < characters; i++)
    {
        if (cmd_data[i] == ' ' && !string) cmd_data[i] = 0;
        if (cmd_data[i] == '\"') 
        {
            string ^= true;
            cmd_data[i] = 0;
        }
    }
    int bytes_left = characters;
    
    char* first_arg = find_first_arg(cmd_data, &bytes_left);
    if (!first_arg)
    {
        return_value = 127;
        goto do_return;
    }

    char* arg = first_arg;
    int bytes_left_backup = bytes_left;
    int argc = 1;
    while (arg = find_next_contiguous_string(arg, &bytes_left))
        argc++;
    bytes_left = bytes_left_backup;

    char** argv = malloc((argc + 1) * sizeof(char*));
    
    if (!argv) 
    {
        return_value = 1;
        goto do_return;
    }

    arg = first_arg;
    argv[0] = arg;
    int i = 1;
    while (arg = find_next_contiguous_string(arg, &bytes_left))
        argv[i++] = arg;
    argv[i] = NULL;

    pid_t ret = fork();
    if (ret == -1)
    {
        return_value = 127;
        goto do_return;
    }

    if (ret == 0)
    {
        execvp(first_arg, argv);
        if (errno == ENOENT)
            fprintf(stderr, "%s: command not found\n", first_arg);
        else
            fprintf(stderr, "%s: couldn't run command\n", first_arg);
        exit(127);
    }
    else
    {
        int wstatus;
        waitpid(ret, &wstatus, 0);
        return_value = WEXITSTATUS(wstatus);

        free(argv);
        goto do_return;
    }

    free(argv);

    fprintf(stderr, "%s: command not found\n", first_arg);

    return_value = 127;
    goto do_return;

do_return:
    free(cmd_data);
    return return_value;
}

char* realpath(const char* path, char* resolved_path)
{
    if (!path) 
    {
        errno = EINVAL;
        return NULL;
    }

    if (!resolved_path)
        resolved_path = malloc(PATH_MAX);
    if (!resolved_path)
    {
        errno = ENOMEM;
        return NULL;
    }

    if (path[0] == 0)
    {
        resolved_path[0] = 0;
        return resolved_path;
    }

    int i = 0, j = 0;
    if (path[0] != '/') 
    {
        if (!getcwd(resolved_path, PATH_MAX)) 
        {
            free(resolved_path);
            return NULL;
        }
        j = strlen(resolved_path);
    } 
    else 
    {
        resolved_path[0] = '/';
        j = 1;
        i = 1;
    }

    while (path[i]) 
    {
        while (path[i] == '/') 
            i++;
        if (!path[i]) break;

        int start = i;
        while (path[i] && path[i] != '/') 
            i++;
        int len = i - start;

        if (len == 1 && path[start] == '.')
            continue;

        if (len == 2 && path[start] == '.' && path[start + 1] == '.') 
        {
            if (j > 1) 
            {
                if (resolved_path[j - 1] == '/') 
                    j--;
                while (j > 0 && resolved_path[j - 1] != '/') 
                    j--;

                if (j == 0)
                    resolved_path[j++] = '/';
            } 
            continue;
        }

        if (j == 0 || resolved_path[j - 1] != '/') 
        {
            if (j >= PATH_MAX - 1) 
            { 
                errno = ENAMETOOLONG; 
                free(resolved_path); 
                return NULL; 
            }
            resolved_path[j++] = '/';
        }

        if (j + len >= PATH_MAX) 
        { 
            errno = ENAMETOOLONG; 
            free(resolved_path); 
            return NULL; 
        }

        memcpy(resolved_path + j, path + start, len);
        j += len;
    }

    if (j == 0) 
    {
        if (PATH_MAX < 2) 
        { 
            errno = ENAMETOOLONG; 
            free(resolved_path); 
            return NULL; 
        }
        resolved_path[0] = '/';
        resolved_path[1] = '\0';
        return resolved_path;
    }

    if (j > 1 && resolved_path[j - 1] == '/') 
        j--;
    resolved_path[j] = '\0';
    return resolved_path;
}