#include "irq.h"

#include "../vfs/table.h"
#include "../ps2/ps2.h"
#include "../pic/apic.h"
#include "../time/ktime.h"
#include "../terminal/textio.h"
#include "../multitasking/multitasking.h"
#include "../multitasking/queue.h"
#include "../util/lambda.h"

void handle_apic_irq(interrupt_registers_t* registers)
{
    uint32_t flags = acquire_spinlock_noint(&sched_lock);
    bool ts = false, sigint = false;
    switch (registers->interrupt_number)
    {
    case APIC_TIMER_INT:
    {
        uint32_t flags = acquire_spinlock_noint(&time_lock);
        resolve_time();
        release_spinlock_noint(&time_lock, flags);

        FATAL("TODO: Implement TSC deadlines");

        // if (multitasking_enabled)
        // {
        //     uint32_t flags = acquire_spinlock_noint(&sched_lock);
        //     __run_it_on_queue(&waiting_for_time_tasks, lambda(void, (thread_t* task)
        //     {
        //         if (task->timeout_deadline == NO_TIMEOUT)
        //             return;
        //         if (task->timeout_deadline >= global_timer)
        //             return;
        //         __task_stop_polling(task);
        //     }));
        //     release_spinlock_noint(&sched_lock, flags);
        // }

        // TODO: Remove the "periodic timer interrupt" design entirely from the kernel
        // if (multitasking_enabled)
        // {
        //     if (multitasking_counter <= 0)
        //     {
        //         multitasking_counter = TASK_SWITCH_DELAY;

        //         ts = true;
        //     }
        //     multitasking_counter -= precise_time_to_milliseconds(GLOBAL_TIMER_INCREMENT);
        // }
        break;
    }

    case APIC_PS2_1_INT:
        handle_ps2_irq(&ts, &sigint);
        break;

    case APIC_PS2_2_INT:
        handle_ps2_irq(&ts, &sigint);
        break;

    default:    // * Spurious interrupt
        return;
    }

    lapic_send_eoi();

    if (sigint)
        __task_send_signal_to_pgrp(SIGINT, tty_foreground_pgrp);

    if (ts)
        switch_task();

    release_spinlock_noint(&sched_lock, flags);
}
