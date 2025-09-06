#pragma once

void handle_irq_0(bool* should_task_switch)
{
    global_timer += PIT_INCREMENT;

    if (should_task_switch) *should_task_switch = false;

    if (time_initialized)
        system_increment_time();
    if (multitasking_enabled)
    {
        multitasking_counter--;
        if (multitasking_counter == 0)
        {
            if (should_task_switch) *should_task_switch = true;
            multitasking_counter = TASK_SWITCH_DELAY / PIT_INCREMENT;
        }
        if (multitasking_counter == 0xff)
            multitasking_counter = TASK_SWITCH_DELAY / PIT_INCREMENT;
    }
}

void pit_channel_0_set_frequency(uint32_t frequency)
{
    uint32_t divider = 1193180 / frequency;
    outb(PIT_MODE_CMD, PIT_BINARY_MODE | PIT_OPERATING_MODE(3) | PIT_ACCESS_MODE_LOHIBYTE | PIT_MODE_CHANNEL_0);            
    io_wait();
    outb(PIT_CHANNEL_0_DATA, divider & 0xff);   
    io_wait();
    outb(PIT_CHANNEL_0_DATA, divider >> 8);   
    io_wait(); 
}

void ksleep(uint32_t milliseconds)
{
    uint32_t target_timer = global_timer + milliseconds;
    while(target_timer > global_timer);
}