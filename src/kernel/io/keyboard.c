#pragma once

void utf32_buffer_init(utf32_buffer_t* buffer)
{
    buffer->characters = (utf32_char_t*)pfa_allocate_page();
    buffer->size = 1024; // 4096 / sizeof(utf32_char_t)
    buffer->put_index = buffer->get_index = 0;
}

void utf32_buffer_destroy(utf32_buffer_t* buffer)
{
    if (buffer->characters)
        pfa_free_page((virtual_address_t)buffer->characters);
    buffer->characters = 0;
    buffer->size = 0;
    buffer->put_index = buffer->get_index = 0;
}

void utf32_buffer_putchar(utf32_buffer_t* buffer, utf32_char_t character)
{
    if (!buffer->characters) return;
    if (get_buffered_characters(*buffer) >= buffer->size - 1) return;     // No space to put characters

    buffer->characters[buffer->put_index] = character;
    buffer->put_index = (buffer->put_index + 1) % buffer->size;
}

utf32_char_t utf32_buffer_getchar(utf32_buffer_t* buffer)
{
    if (!buffer->characters) return 0;
    if (no_buffered_characters(*buffer)) return 0;     // No characters to get

    utf32_char_t ch = buffer->characters[buffer->get_index];
    buffer->get_index = (buffer->get_index + 1) % buffer->size;
    return ch;
}

void utf32_buffer_clear(utf32_buffer_t* buffer)
{
    buffer->get_index = buffer->put_index = 0;
}

bool keyboard_is_key_pressed(virtual_address_t vk)
{
    return ps2_kb_is_key_pressed(vk);
}

void keyboard_handle_character(utf32_char_t character)
{
    for (uint16_t i = 0; i < task_count; i++)
    {
        if (tasks[i].reading_stdin)
        {
            utf32_buffer_putchar(&tasks[i].input_buffer, character);
            
            putchar(utf32_to_bios_oem(character));
            
            if (character == '\n')
            {
                tasks[i].reading_stdin = false;
                tasks[i].was_reading_stdin = true;
            }
        }
    }
}