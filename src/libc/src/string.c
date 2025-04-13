void* memset(void* ptr, int value, size_t num)
{
    uint8_t* p = (uint8_t*)ptr;
    for (size_t i = 0; i < num; i++)
        p[i] = value;
    return ptr;
}

void* memcpy(void* destination, const void* source, size_t num)
{
    uint8_t* d = (uint8_t*)destination;
    uint8_t* s = (uint8_t*)source;
    for (size_t i = 0; i < num; i++)
        d[i] = s[i];
    return destination;
}

int memcmp(const void* str1, const void* str2, size_t n)
{
    for(uint64_t i = 0; i < n; i++)
    {
        if(((uint8_t*)str1)[i] < ((uint8_t*)str2)[i])
            return -1;
        if(((uint8_t*)str1)[i] > ((uint8_t*)str2)[i])
            return 1;
    }

    return 0; 
}

int strcmp(const char* str1, const char* str2)
{
    const unsigned char *p1 = (const unsigned char *)str1;
    const unsigned char *p2 = (const unsigned char *)str2;

    while (*p1 && (*p1 == *p2)) 
    {
        p1++; 
        p2++; 
    }

    return (*p1 > *p2) - (*p2 > *p1);
}

size_t strlen(const char* str)
{
    size_t len = 0;
    while (*str)
    {
        str++;
        len++;
    }
    return len;
}