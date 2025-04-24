#pragma once

typedef uint32_t utf32_char_t;

typedef struct utf32_buffer
{
    utf32_char_t* characters;
    size_t size;
    size_t put_index, get_index;
} utf32_buffer_t;

#define get_buffered_characters(buffer) imod(((int)(buffer).put_index - (buffer).get_index), (buffer).size)
#define no_buffered_characters(buffer)  ((buffer).put_index == (buffer).get_index)

#define utf32_to_bios_oem(utf32_char)   (utf32_char < 128 ? (char)utf32_char : 0) // Note: Other characters are code page dependent

void utf32_buffer_init(utf32_buffer_t* buffer);
void utf32_buffer_destroy(utf32_buffer_t* buffer);
void utf32_buffer_putchar(utf32_buffer_t* buffer, utf32_char_t character);
utf32_char_t utf32_buffer_getchar(utf32_buffer_t* buffer);