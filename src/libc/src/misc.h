#pragma once

static char* find_next_contiguous_string(char* str, int* bytes_left)
{
    if (!str) return NULL;
    if (!bytes_left) return NULL;
    while (*str && (*bytes_left) > 0)
    {
        str++;
        (*bytes_left)--;
    }
    while (!(*str) && (*bytes_left) > 0)
    {
        str++;
        (*bytes_left)--;
    }
    if ((*bytes_left) <= 0) return NULL;
    return str;
}

static char* find_first_arg(char* data, int* bytes_left)
{
    return data[0] ? data : find_next_contiguous_string(data, bytes_left);
}