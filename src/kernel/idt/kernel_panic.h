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

    #define TRACE_FUNCTIONS
    #ifndef TRACE_FUNCTIONS
    for (int i = 0; i < 8; i++)
    {
        uint32_t* ptr = !(tasks[current_task_index].ring != 0 && multitasking_enabled && !first_task_switch) ? 
            &((uint32_t*)&registers[1])[i] : 
            &((uint32_t*)((struct privilege_switch_interrupt_registers*)registers)->esp)[i];

        printf("esp + %u (0x%x) : 0x%x\n", i * 4, ptr, *ptr);
    }
    #else
    typedef struct __attribute__((packed)) call_frame
    {
        uint32_t ebp;
        uint32_t eip;
    } call_frame_t;

    call_frame_t* ebp = (call_frame_t*)registers->ebp;
    // asm volatile ("mov eax, ebp" : "=a"(ebp));

    // ~ Log the last function (the one the exception happened in)
    printf("eip : 0x");
    tty_set_color(FG_YELLOW, BG_BLACK);
    printf("%x", registers->eip);
    tty_set_color(FG_WHITE, BG_BLACK);
    putchar(' ');

    LOG(DEBUG, "eip : 0x%x ", registers->eip);
    print_kernel_symbol_name(registers->eip, (uint32_t)ebp);
    putchar('\n');

    int i = 0;
    const int max_stack_frames = 12;

    while (i <= max_stack_frames && (uint32_t)ebp != 0 && (uint32_t)ebp->eip != 0 && 
    (((!multitasking_enabled || (multitasking_enabled && first_task_switch)) &&
            (((uint32_t)ebp > (uint32_t)&stack_bottom) && ((uint32_t)ebp <= (uint32_t)&stack_top))) || 
        (((uint32_t)ebp < TASK_STACK_TOP_ADDRESS) && ((uint32_t)ebp >= TASK_STACK_BOTTOM_ADDRESS))))
    {
        if (i == max_stack_frames)
        {
            tty_set_color(FG_RED, BG_BLACK);
            printf("...");
            tty_set_color(FG_WHITE, BG_BLACK);
            LOG(DEBUG, "...");
        }
        else
        {
            printf("eip : 0x%x | ebp : 0x%x ", ebp->eip - 4, ebp);
            LOG(DEBUG, "eip : 0x%x | ebp : 0x%x ", ebp->eip - 4, ebp);
            print_kernel_symbol_name(ebp->eip - 4, (uint32_t)ebp); // ^ -4 because it pushes the return address not the calling address
            putchar('\n');
            ebp = (call_frame_t*)ebp->ebp;
        }
        i++;
    }
    #endif

    halt();
}