#pragma once

void ps2_kb_get_scancode_set()
{
    if (PS2_DEVICE_1_KB)
    {
        ps2_send_device_command(1, PS2_KB_GET_SET_SCANCODE_SET);
        ps2_flush_buffer();
        ps2_send_device_command(1, 0);  // GET
        ps2_read_data();
        ps2_kb_1_scancode_set = ps2_data_buffer[0] == PS2_ACK ? ps2_data_buffer[1] : 0xff;

        ps2_print_read_buffer();
    }
    else
        ps2_kb_1_scancode_set = 0xff;

    if (PS2_DEVICE_2_KB)
    {
        ps2_send_device_command(2, PS2_KB_GET_SET_SCANCODE_SET);
        ps2_flush_buffer();
        ps2_send_device_command(2, 0);  // GET
        ps2_read_data();
        ps2_kb_2_scancode_set = ps2_data_buffer[0] == PS2_ACK ? ps2_data_buffer[1] : 0xff;
    }
    else
        ps2_kb_2_scancode_set = 0xff;
}

void ps2_init_keyboards()
{
    // ps2_flush_buffer();
    
    ps2_kb_get_scancode_set();

    if (PS2_DEVICE_1_KB)
    {
        LOG(DEBUG, "Keyboard 1 scancode set : %u", ps2_kb_1_scancode_set);
        kprintf("Keyboard 1 scancode set : %u\n", ps2_kb_1_scancode_set);
    }

    if (PS2_DEVICE_2_KB)
    {
        LOG(DEBUG, "Keyboard 2 scancode set : %u", ps2_kb_2_scancode_set);
        kprintf("Keyboard 2 scancode set : %u\n", ps2_kb_2_scancode_set);
    }
}

void ps2_handle_keyboard_scancode(uint8_t port, uint8_t scancode) 
{
    if (port == 1)
        if (ps2_kb_1_scancode_set != 2)
            return;
    if (port == 2)
        if (ps2_kb_2_scancode_set != 2)
            return;
    // if (!(scancode & 0x80))
        kprintf("A");
}