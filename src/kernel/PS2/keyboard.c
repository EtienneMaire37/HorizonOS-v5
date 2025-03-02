#pragma once

void ps2_handle_keyboard_scancode(uint8_t port, uint8_t scancode) 
{
    if (!(scancode & 0x80))
        kprintf("A");
}