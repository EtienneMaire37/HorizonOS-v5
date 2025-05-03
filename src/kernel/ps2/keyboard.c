#pragma once

void ps2_kb_get_scancode_set()
{
    if (PS2_DEVICE_1_KB)
    {
        ps2_send_device_full_command_with_data(1, PS2_KB_GET_SET_SCANCODE_SET, 2, 1);
        if(ps2_send_device_full_command_with_data(1, PS2_KB_GET_SET_SCANCODE_SET, 0, 2))
            ps2_kb_1_scancode_set = ps2_data_buffer[0] == PS2_ACK ? ps2_data_buffer[1] : 0xff;
        else
            ps2_kb_1_scancode_set = 0xff;
    }
    else
        ps2_kb_1_scancode_set = 0xff;

    if (PS2_DEVICE_2_KB)
    {
        ps2_send_device_full_command_with_data(2, PS2_KB_GET_SET_SCANCODE_SET, 2, 1);
        if(ps2_send_device_full_command_with_data(2, PS2_KB_GET_SET_SCANCODE_SET, 0, 2))
            ps2_kb_2_scancode_set = ps2_data_buffer[0] == PS2_ACK ? ps2_data_buffer[1] : 0xff;
        else
            ps2_kb_2_scancode_set = 0xff;
    }
    else
        ps2_kb_2_scancode_set = 0xff;
}

void ps2_init_keyboards()
{
    ps2_kb_get_scancode_set();

    if (PS2_DEVICE_1_KB)
    {
        LOG(DEBUG, "Keyboard 1 scancode set : %u", ps2_kb_1_scancode_set);
        printf("Keyboard 1 scancode set : %u\n", ps2_kb_1_scancode_set);
    }

    if (PS2_DEVICE_2_KB)
    {
        LOG(DEBUG, "Keyboard 2 scancode set : %u", ps2_kb_2_scancode_set);
        printf("Keyboard 2 scancode set : %u\n", ps2_kb_2_scancode_set);
    }
}

utf32_char_t ps2_scancode_to_unicode(ps2_full_scancode_t scancode, uint8_t port)
{
    bool num_lock = ps2_kb_num_lock[port - 1];
    bool caps_lock = ps2_kb_caps_lock[port - 1];
    bool shift = ps2_kb_is_key_pressed(VK_LSHIFT) || ps2_kb_is_key_pressed(VK_RSHIFT);
    bool altgr = ps2_kb_is_key_pressed(VK_RALT);

    bool is_keypad = false;
    utf32_char_t keypad_char = 0;
    const uint8_t keypad_scancodes[] = {0x70, 0x69, 0x72, 0x7A, 0x6B, 0x73, 0x74, 0x6C, 0x75, 0x7D};
    const utf32_char_t keypad_chars[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};

    if (!scancode.extended) 
    {
        for (int i = 0; i < 10; i++) 
        {
            if (scancode.scancode == keypad_scancodes[i]) 
            {
                is_keypad = true;
                keypad_char = keypad_chars[i];
                break;
            }
        }
    }

    utf32_char_t base_char = 0;
    if (is_keypad && num_lock && !shift) 
    {
        base_char = keypad_char;
    } 
    else 
    {
        base_char = scancode.extended 
            ? current_keyboard_layout->ps2_layout_data.char_table_e0[scancode.scancode]
            : current_keyboard_layout->ps2_layout_data.char_table[scancode.scancode];
    }

    utf32_char_t shifted_char = current_keyboard_layout->ps2_layout_data.char_table_shift[scancode.scancode];
    
    if (is_keypad) 
    {
        if (shift && num_lock) 
            return 0;
        if (!num_lock && !shift) 
            return 0;
    }

    if (base_char >= 'a' && base_char <= 'z') 
        return (caps_lock ^ shift) ? (base_char - 32) : base_char;

    if (altgr) 
    {
        utf32_char_t altgr_char = current_keyboard_layout->ps2_layout_data.char_table_altgr[scancode.scancode];
        if (altgr_char != 0) return altgr_char;
    }

    if (shift && shifted_char != 0)
        return shifted_char;

    return base_char;
}

void ps2_handle_keyboard_scancode(uint8_t port, uint8_t scancode)   // port is 1-2
{
    if (port == 1)
        if (ps2_kb_1_scancode_set != 2)
            return;
    if (port == 2)
        if (ps2_kb_2_scancode_set != 2)
            return;

    uint8_t port_index = port - 1;

    if (scancode == 0xf0)           // release code
        current_ps2_keyboard_scancodes[port_index].release = true;
    else if (scancode == 0xe0)      // extended code
        current_ps2_keyboard_scancodes[port_index].extended = true;
    else
    {
        current_ps2_keyboard_scancodes[port_index].scancode = scancode;

        if (current_ps2_keyboard_scancodes[port_index].release)
        {
            if (current_ps2_keyboard_scancodes[port_index].extended)
                ps2_keyboard_state_e0[current_ps2_keyboard_scancodes[port_index].scancode] &= ~(1 << port_index);
            else
                ps2_keyboard_state[current_ps2_keyboard_scancodes[port_index].scancode] &= ~(1 << port_index);
        }
        else
        {
            if (current_ps2_keyboard_scancodes[port_index].extended)
                ps2_keyboard_state_e0[current_ps2_keyboard_scancodes[port_index].scancode] |= (1 << port_index);
            else
                ps2_keyboard_state[current_ps2_keyboard_scancodes[port_index].scancode] |= (1 << port_index);
            virtual_key_t vk = current_ps2_keyboard_scancodes[port_index].extended ? current_keyboard_layout->ps2_layout_data.vk_table_e0[current_ps2_keyboard_scancodes[port_index].scancode] : current_keyboard_layout->ps2_layout_data.vk_table[current_ps2_keyboard_scancodes[port_index].scancode];
            if (!current_ps2_keyboard_scancodes[port_index].release)
            {
                switch (vk)
                {
                case VK_CAPSLOCK:
                    ps2_kb_caps_lock[port - 1] ^= 1;
                    break;
                case VK_SCROLLLOCK:
                    ps2_kb_scroll_lock[port - 1] ^= 1;
                    break;
                case VK_NUMLOCK:
                    ps2_kb_num_lock[port - 1] ^= 1;
                    break;
                default:
                    ;
                }
            }

            ps2_kb_update_leds(port);

            utf32_char_t character = ps2_scancode_to_unicode(current_ps2_keyboard_scancodes[port_index], port);
            keyboard_handle_character(character);
        }

        current_ps2_keyboard_scancodes[port_index].release = current_ps2_keyboard_scancodes[port_index].extended = false;
    }
}

bool ps2_kb_is_key_pressed_with_port(virtual_key_t vk, uint8_t port)  // Port of the ps/2 keyboard
{
    if (port == 0 || port > 2) return false;

    for (uint16_t i = 0; i < 256; i++)
    {
        if ((current_keyboard_layout->ps2_layout_data.vk_table[i] == vk) && (ps2_keyboard_state[i] & (1 << (port - 1))))
            return true;
    }
    for (uint16_t i = 0; i < 256; i++)
    {
        if ((current_keyboard_layout->ps2_layout_data.vk_table_e0[i] == vk) && (ps2_keyboard_state_e0[i] & (1 << (port - 1))))
            return true;
    }
    return false;
}

bool ps2_kb_is_key_pressed(virtual_key_t vk)
{
    for (uint16_t i = 0; i < 256; i++)
    {
        if ((current_keyboard_layout->ps2_layout_data.vk_table[i] == vk) && (ps2_keyboard_state[i] & 0b11))
            return true;
    }
    for (uint16_t i = 0; i < 256; i++)
    {
        if ((current_keyboard_layout->ps2_layout_data.vk_table_e0[i] == vk) && (ps2_keyboard_state_e0[i] & 0b11))
            return true;
    }
    return false;
}

void ps2_kb_update_leds(uint8_t port)
{
    if (port == 0 || port > 2) return;

    bool num_lock = ps2_kb_num_lock[port - 1];
    bool caps_lock = ps2_kb_caps_lock[port - 1];
    bool scroll_lock = ps2_kb_scroll_lock[port - 1];
    
    uint8_t leds = (caps_lock << 2) | (num_lock << 1) | scroll_lock;

    ps2_send_device_full_command_with_data(port, PS2_KB_SET_LED_STATE, leds, 1);
}