#pragma once

static uint32_t multitasking_second_counter = 0;
static int16_t multitasking_counter = 0;

void handle_irq_0(bool* should_task_switch)
{
    global_timer += PIT_INCREMENT;

    if (should_task_switch) *should_task_switch = false;

    if (time_initialized)
        system_increment_time();
    if (multitasking_enabled)
    {
        multitasking_counter--;
        if (multitasking_counter <= 0)
        {
            if (should_task_switch) *should_task_switch = true;
            multitasking_counter = TASK_SWITCH_DELAY / precise_time_to_milliseconds(PIT_INCREMENT);
        }
        if (multitasking_counter == 0xff)
            multitasking_counter = TASK_SWITCH_DELAY / precise_time_to_milliseconds(PIT_INCREMENT);

        tasks[current_task_index].current_cpu_ticks += precise_time_to_milliseconds(PIT_INCREMENT);
        multitasking_second_counter += precise_time_to_milliseconds(PIT_INCREMENT);

        if (multitasking_second_counter >= 1000) 
        {
            multitasking_second_counter = 0;

            if (task_count != 0)
            {
                for (uint16_t i = 0; i < task_count; i++)
                {
                    tasks[i].stored_cpu_ticks = tasks[i].current_cpu_ticks;
                    tasks[i].current_cpu_ticks = 0;
                }

                int32_t total = 1000 - tasks[0].stored_cpu_ticks;
                #define CLAMP_CPU_USAGE
                #ifdef CLAMP_CPU_USAGE
                if (total <= 0) total = 0;
                if (total >= 1000) total = 1000;
                #endif

                LOG(TRACE, "CPU usage:");
                LOG(TRACE, "total : %d.%d %%", total / 10, total % 10);
                for (uint16_t i = 0; i < task_count; i++)
                {
                    int32_t this_percentage = tasks[i].stored_cpu_ticks;
                    #ifdef CLAMP_CPU_USAGE
                    if (this_percentage <= 0) this_percentage = 0;
                    if (this_percentage >= 1000) this_percentage = 1000;
                    #endif
                    LOG(TRACE, "%s── task %d : %d.%d %%\t[pid = %ld]%s%s", task_count - i > 1 ? "├" : "└", i, this_percentage / 10, this_percentage % 10, tasks[i].pid, i == 0 ? " (* idle task *)" : "", tasks[i].is_dead ? " [waiting for deletion...]" : "");
                }
            }
        }
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

void ksleep(precise_time_t time)
{
    precise_time_t target_timer = global_timer + time;
    while(target_timer > global_timer);
}