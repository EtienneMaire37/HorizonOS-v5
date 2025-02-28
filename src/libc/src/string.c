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