#include "string.h"

int strcmp_slash(const char* str1, const char* str2)
{
    while (*str1 && *str2 && *str1 != '/' && *str2 != '/')
    {
        if (*str1 != *str2)
            return (int)*str1 - *str2;
        
        str1++;
        str2++;
    }
    return ((!*str1 || *str1 == '/') && (!*str2 || *str2 == '/')) ? 0 : (int)*str1 - *str2;
}

size_t strlen_slash(const char* str)
{
    size_t len = 0;
    while (*str && *str != '/')
    {
        str++;
        len++;
    }
    return len;
}

bool str_starts_with(const char* str, const char* prefix)
{
    while (*str && *prefix && *str == *prefix)
    {
        str++;
        prefix++;
    }
    return !*prefix;
}
