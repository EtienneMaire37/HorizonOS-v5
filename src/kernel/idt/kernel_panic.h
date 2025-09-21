#pragma once

void kernel_panic(struct interrupt_registers* registers)
{
    disable_interrupts();

    LOG(CRITICAL, "Kernel panic");

    tty_set_color(FG_WHITE, BG_BLACK);
    tty_clear_screen(' ');

    tty_cursor = 0;
    tty_update_cursor();
    // tty_hide_cursor();

    tty_set_color(FG_LIGHTRED, BG_BLACK);
    printf("Kernel panic\n\n");

    tty_set_color(FG_LIGHTGREEN, BG_BLACK);

    if (multitasking_enabled)
        printf("Task : \"%s\" (pid = %lu) | ring = %u\n\n", tasks[current_task_index].name, tasks[current_task_index].pid, tasks[current_task_index].ring);

    tty_set_color(FG_WHITE, BG_BLACK);

    const char* error_message = get_error_message(registers->interrupt_number, registers->error_code);

    printf("Exception number: %u\n", registers->interrupt_number);
    printf("Error:       ");
    tty_set_color(FG_YELLOW, BG_BLACK);
    puts(error_message);
    tty_set_color(FG_WHITE, BG_BLACK);
    printf("Error code:  0x%x\n\n", registers->error_code);

    if (registers->interrupt_number == 14)
    {
        printf("cr2:  0x%x (pde %u pte %u offset 0x%x)\n", registers->cr2, registers->cr2 >> 22, (registers->cr2 >> 12) & 0x3ff, registers->cr2 & 0xfff);
        printf("cr3:  0x%x\n\n", registers->cr3);

        uint32_t pde = read_physical_address_4b(registers->cr3 + 4 * (registers->cr2 >> 22));
        
        LOG(DEBUG, "Page directory entry : 0x%x", pde);

        if (pde & 1)
        {
            LOG(DEBUG, "Page table entry : 0x%x", read_physical_address_4b((pde & 0xfffff000) + 4 * ((registers->cr2 >> 12) & 0x3ff)));
        }
    }

    printf("Stack trace : \n");
    LOG(DEBUG, "Stack trace : ");

    typedef struct call_frame
    {
        struct call_frame* ebp;
        uint32_t eip;
    } call_frame_t;

    call_frame_t* ebp = (call_frame_t*)registers->ebp;
    // asm volatile ("mov eax, ebp" : "=a"(ebp));

    // ~ Log the last function (the one the exception happened in)
    printf("eip : 0x%x ", registers->eip);
    LOG(DEBUG, "eip : 0x%x ", registers->eip);
    if ((((uint32_t)ebp > (uint32_t)&stack_bottom) && ((uint32_t)ebp <= (uint32_t)&stack_top)) || 
    (((uint32_t)ebp > TASK_STACK_BOTTOM_ADDRESS) && ((uint32_t)ebp <= TASK_STACK_TOP_ADDRESS)))
        print_kernel_symbol_name(registers->eip, (uint32_t)ebp);
    putchar('\n');

    while ((uint32_t)ebp != 0 && (!multitasking_enabled || (multitasking_enabled && first_task_switch) || ((uint32_t)ebp < 0xc0000000) && ((uint32_t)ebp >= 767U * 1024 * 4096)))
    {
        printf("eip : 0x%x | ebp : 0x%x ", ebp->eip, ebp);
        LOG(DEBUG, "eip : 0x%x | ebp : 0x%x ", ebp->eip, ebp);
        if ((((uint32_t)ebp > (uint32_t)&stack_bottom) && ((uint32_t)ebp <= (uint32_t)&stack_top)) || 
        (((uint32_t)ebp > TASK_STACK_BOTTOM_ADDRESS) && ((uint32_t)ebp <= TASK_STACK_TOP_ADDRESS)))
            print_kernel_symbol_name(ebp->eip - 1, (uint32_t)ebp); // ^ -1 because it pushes the return address not the calling address
        putchar('\n');
        ebp = ebp->ebp;
    }

    halt();
}