#pragma once

void utf32_buffer_init(utf32_buffer_t* buffer)
{
    buffer->characters = (utf32_char_t*)physical_address_to_virtual(pfa_allocate_physical_page());
    buffer->size = 1024; // 4096 / sizeof(utf32_char_t)
    buffer->put_index = buffer->get_index = 0;
}

void utf32_buffer_destroy(utf32_buffer_t* buffer)
{
    if (buffer->characters)
        pfa_free_physical_page(virtual_address_to_physical((virtual_address_t)buffer->characters));   // !!! buffer must be allocated below 1GB
    buffer->characters = 0;
    buffer->size = 0;
    buffer->put_index = buffer->get_index = 0;
}

void utf32_buffer_copy(utf32_buffer_t* from, utf32_buffer_t* to)
{
    utf32_buffer_init(to);
    memcpy(to->characters, from->characters, 4096);
    to->put_index = from->put_index;
}

void utf32_buffer_putchar(utf32_buffer_t* buffer, utf32_char_t character)
{
    if (!buffer->characters) return;
    if (get_buffered_characters(*buffer) >= buffer->size - 1) return;     // No space to put characters

    buffer->characters[buffer->put_index] = character;
    buffer->put_index = (buffer->put_index + 1) % buffer->size;
}

void utf32_buffer_unputchar(utf32_buffer_t* buffer)
{
    if (!buffer->characters) return;
    if (no_buffered_characters(*buffer)) return;

    buffer->put_index = imod(buffer->put_index - 1, buffer->size);
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

void keyboard_handle_character(utf32_char_t character, virtual_key_t vk)
{
    char ascii = utf32_to_bios_oem(character);
    if (ascii == 0
        && vk != VK_UP && vk != VK_RIGHT && vk != VK_DOWN && vk != VK_LEFT
        && vk != VK_HOME && vk != VK_END
        && vk != VK_INSERT && vk != VK_DELETE
        && vk != VK_PAGEUP && vk != VK_PAGEDOWN
        ) 
        return;
    for (uint16_t i = 0; i < task_count; i++)
    {
        if (tasks[i].reading_stdin)
        {            
            if (character == '\b')
            {
                if (!no_buffered_characters(tasks[i].input_buffer))
                {
                    putchar('\b');
                    utf32_buffer_unputchar(&tasks[i].input_buffer);
                }
            }
            else
            {
                size_t num_characters = get_buffered_characters(tasks[i].input_buffer);
                size_t max_characters = tasks[i].input_buffer.size - 1;
                size_t character_len;

                switch (vk)
                {
                case VK_TAB:    character_len = TAB_LENGTH; break;

                case VK_UP:     character_len = 3; break;
                case VK_DOWN:   character_len = 3; break;
                case VK_RIGHT:  character_len = 3; break;
                case VK_LEFT:   character_len = 3; break;

                case VK_HOME:   character_len = 3; break;
                case VK_END:    character_len = 3; break;

                case VK_INSERT:   character_len = 4; break;
                case VK_DELETE:   character_len = 4; break;

                case VK_PAGEUP:   character_len = 4; break;
                case VK_PAGEDOWN: character_len = 4; break;

                default:        character_len = 1 + keyboard_is_key_pressed(VK_LALT);
                }

                if ((ssize_t)num_characters < (ssize_t)max_characters - character_len)
                {
                    switch (vk)
                    {
                    case VK_TAB:
                        for (uint8_t j = 0; j < TAB_LENGTH; j++)
                        {
                            utf32_buffer_putchar(&tasks[i].input_buffer, ' ');                    
                            putchar(' ');
                        }
                        break;
                    case VK_UP:
                        utf32_buffer_putchar(&tasks[i].input_buffer, '\x1b');
                        utf32_buffer_putchar(&tasks[i].input_buffer, '[');
                        utf32_buffer_putchar(&tasks[i].input_buffer, 'A');
                        printf("^[A");
                        break;
                    case VK_DOWN:
                        utf32_buffer_putchar(&tasks[i].input_buffer, '\x1b');
                        utf32_buffer_putchar(&tasks[i].input_buffer, '[');
                        utf32_buffer_putchar(&tasks[i].input_buffer, 'B');
                        printf("^[B");
                        break;
                    case VK_RIGHT:
                        utf32_buffer_putchar(&tasks[i].input_buffer, '\x1b');
                        utf32_buffer_putchar(&tasks[i].input_buffer, '[');
                        utf32_buffer_putchar(&tasks[i].input_buffer, 'C');
                        printf("^[C");
                        break;
                    case VK_LEFT:
                        utf32_buffer_putchar(&tasks[i].input_buffer, '\x1b');
                        utf32_buffer_putchar(&tasks[i].input_buffer, '[');
                        utf32_buffer_putchar(&tasks[i].input_buffer, 'D');
                        printf("^[D");
                        break;
                    case VK_HOME:
                        utf32_buffer_putchar(&tasks[i].input_buffer, '\x1b');
                        utf32_buffer_putchar(&tasks[i].input_buffer, '[');
                        utf32_buffer_putchar(&tasks[i].input_buffer, 'H');
                        printf("^[H");
                        break;
                    case VK_END:
                        utf32_buffer_putchar(&tasks[i].input_buffer, '\x1b');
                        utf32_buffer_putchar(&tasks[i].input_buffer, '[');
                        utf32_buffer_putchar(&tasks[i].input_buffer, 'F');
                        printf("^[F");
                        break;
                    case VK_INSERT:
                        utf32_buffer_putchar(&tasks[i].input_buffer, '\x1b');
                        utf32_buffer_putchar(&tasks[i].input_buffer, '[');
                        utf32_buffer_putchar(&tasks[i].input_buffer, '2');
                        utf32_buffer_putchar(&tasks[i].input_buffer, '~');
                        printf("^[2~");
                        break;
                    case VK_DELETE:
                        utf32_buffer_putchar(&tasks[i].input_buffer, '\x1b');
                        utf32_buffer_putchar(&tasks[i].input_buffer, '[');
                        utf32_buffer_putchar(&tasks[i].input_buffer, '3');
                        utf32_buffer_putchar(&tasks[i].input_buffer, '~');
                        printf("^[3~");
                        break;
                    case VK_PAGEUP:
                        utf32_buffer_putchar(&tasks[i].input_buffer, '\x1b');
                        utf32_buffer_putchar(&tasks[i].input_buffer, '[');
                        utf32_buffer_putchar(&tasks[i].input_buffer, '5');
                        utf32_buffer_putchar(&tasks[i].input_buffer, '~');
                        printf("^[5~");
                        break;
                    case VK_PAGEDOWN:
                        utf32_buffer_putchar(&tasks[i].input_buffer, '\x1b');
                        utf32_buffer_putchar(&tasks[i].input_buffer, '[');
                        utf32_buffer_putchar(&tasks[i].input_buffer, '6');
                        utf32_buffer_putchar(&tasks[i].input_buffer, '~');
                        printf("^[6~");
                        break;
                    default:
                        if (ascii)
                        {
                            if (keyboard_is_key_pressed(VK_LALT))
                            {
                                utf32_buffer_putchar(&tasks[i].input_buffer, '\x1b');                    
                                putchar('^');
                            }
                            utf32_buffer_putchar(&tasks[i].input_buffer, character);                    
                            putchar(ascii);
                        }
                    }
                    
                    if (character == '\n')
                    {
                        tasks[i].reading_stdin = false;
                        tasks[i].was_reading_stdin = true;
                    }
                }
            }
        }
    }

    fflush(stdout);
}