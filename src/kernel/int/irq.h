#pragma once

#include "int.h"

void handle_apic_irq(interrupt_registers_t* registers)
{
    switch (registers->interrupt_number)
    {
    case 0x80:  // * APIC Timer
    {
        uint64_t increment = precise_time_to_milliseconds(GLOBAL_TIMER_INCREMENT);
        global_timer += GLOBAL_TIMER_INCREMENT;
        system_thousands += increment;
        resolve_time();

        if (system_thousands - increment < 500 && system_thousands >= 500)
        {
            tty_cursor_blink ^= true;
            if (tty_cursor_blink)
            {
                tty_render_cursor(tty_cursor);
            }
            else
            {
                uint16_t data = tty_data[tty_cursor];
                tty_render_character(tty_cursor, data, data >> 8);
            }
        }
        break;
    }

    default:    // * Spurious interrupt
        return;
    }

    lapic_send_eoi();
}