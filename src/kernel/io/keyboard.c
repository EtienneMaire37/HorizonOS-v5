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
    if (get_buffered_characters(*buffer) == buffer->size - 1) return;     // No space to put characters

    buffer->characters[buffer->put_index++] = character;
    buffer->put_index %= buffer->size;
}

utf32_char_t utf32_buffer_getchar(utf32_buffer_t* buffer)
{
    if (!buffer->characters) return 0;
    if (no_buffered_characters(*buffer)) return 0;     // No characters to get
    size_t _get_index = buffer->get_index;
    buffer->get_index = (buffer->get_index + 1) % buffer->size;
    return buffer->characters[_get_index];
}

void keyboard_handle_character(utf32_char_t character)
{
    for (uint16_t i = 0; i < task_count; i++)
    {
        if (tasks[i].reading_stdin)
        {
            if (character == '\n')
            {
                tasks[i].reading_stdin = false;
                uint32_t _eax = min(get_buffered_characters(tasks[current_task_index].input_buffer), tasks[i].registers_data.edx);
                task_write_register_data(&tasks[i], eax, _eax);
                for (uint32_t i = 0; i < _eax; i++)
                {
                    // *** Only ASCII for now ***
                    ((char*)tasks[i].registers_data.ecx)[i] = utf32_to_bios_oem(utf32_buffer_getchar(&tasks[current_task_index].input_buffer));
                }
            }
            else
                utf32_buffer_putchar(&tasks[i].input_buffer, character);
            putchar(utf32_to_bios_oem(character));
        }
    }
}