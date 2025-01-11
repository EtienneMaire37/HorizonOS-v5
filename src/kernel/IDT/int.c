#pragma once

void kernel_panic(struct interrupt_registers params)
{
    disable_interrupts();

    tty_color = BG_BLUE;
    tty_clear_screen(' ');

    tty_cursor = 4 + 2 * 80;
    tty_update_cursor();
    tty_hide_cursor();

    tty_color = (FG_LIGHTRED | BG_BLUE);
    kfprintf(kstdout, "Kernel panic\n\n\t");

    tty_color = (FG_WHITE | BG_BLUE);

    kfprintf(kstdout, "Exception number: %u\n\n\t", params.interruptNumber);
    kfprintf(kstdout, "Error:       %s\n\t", errorString[params.interruptNumber]);
    kfprintf(kstdout, "Error code:  0x%x\n\n\t", params.errorCode);

    kfprintf(kstdout, "cr2:  0x%x\n", params.cr2);

    // LOG(ERROR, "Kernel panic : Exception number : %u ; Error : %s ; Error code = 0x%x", params.interruptNumber, errorString[params.interruptNumber], params.errorCode);

    halt();
}

// #define return_from_isr() { return task_data_segment(*currentTask); }
#define return_from_isr() { return; }

void interrupt_handler(struct interrupt_registers params)
{
    if(params.interruptNumber < 32)            // Fault
    {
        LOG(ERROR, "Fault : Exception number : %u ; Error : %s ; Error code = 0x%x ; cr2 = 0x%x", params.interruptNumber, errorString[params.interruptNumber], params.errorCode, params.cr2);

        kernel_panic(params);
    }
    else if(params.interruptNumber < 32 + 16)  // IRQ
    {
        uint8_t irqNumber = params.interruptNumber - 32;

        if(irqNumber == 7 && !(pic_get_isr() >> 7))
            return_from_isr();
        if(irqNumber == 15 && !(pic_get_isr() >> 15))
        {
            outb(PIC1_CMD, PIC_EOI);
	        io_wait();
            return_from_isr();
        }
        
        // LOG(INFO, "Interrupt %u handled", params.interruptNumber);

        switch (irqNumber)
        {
        case 0:
            handle_irq_0();
            break;

        default:
            break;
        }

        pic_send_eoi(irqNumber);
        return_from_isr();
    }

    return_from_isr();
}