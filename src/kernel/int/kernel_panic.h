#pragma once

#define is_a_valid_function(symbol_type) ((symbol_type) == 'T' || (symbol_type) == 'R' || (symbol_type) == 't' || (symbol_type) == 'r')  

void print_kernel_symbol_name(uint32_t eip, uint32_t ebp)
{
    if (ebp <= 0xc0000000 && ebp != 0) return;
    initrd_file_t* file = kernel_symbols_file;
    if (file == NULL) return;
    
    uint32_t symbol_address = 0, last_symbol_address = 0, current_symbol_address = 0;
    uint32_t file_offset = 0, line_offset = 0;
    char last_symbol_buffer[64] = {0};
    uint16_t last_symbol_buffer_length = 0;
    char current_symbol_type = ' ', found_symbol_type = ' ';
    while (file_offset < file->size)
    {
        char ch = file->data[file_offset];
        if (ch == '\n')
        {
            if (last_symbol_address <= eip && symbol_address > eip && is_a_valid_function(found_symbol_type))
            {
                putchar(file == kernel_symbols_file ? '[' : '(');
                CONTINUE_LOG(DEBUG, file == kernel_symbols_file ? "[" : "(");
                
                if (found_symbol_type == 'T' || found_symbol_type == 't')
                    tty_set_color(FG_LIGHTCYAN, BG_BLACK);
                else if (found_symbol_type == 'R' || found_symbol_type == 'r')
                    tty_set_color(FG_LIGHTMAGENTA, BG_BLACK);
                else
                    tty_set_color(FG_LIGHTGRAY, BG_BLACK);

                uint8_t light_tty_color = tty_color;
                bool subfunction = false;

                for (uint8_t i = 0; i < minint(64, last_symbol_buffer_length); i++)
                {
                    if (last_symbol_buffer[i] == '.')
                    {
                        tty_set_color(FG_LIGHTGRAY, BG_BLACK);
                        subfunction = true;
                    }
                    putchar(last_symbol_buffer[i]);
                    CONTINUE_LOG(DEBUG, "%c", last_symbol_buffer[i]);
                    if (subfunction)
                    {
                        tty_set_color(light_tty_color & 0x07, light_tty_color & 0x70);
                        subfunction = false;
                    }
                }
                tty_set_color(FG_WHITE, BG_BLACK);
                putchar(file == kernel_symbols_file ? ']' : ')');
                CONTINUE_LOG(DEBUG, file == kernel_symbols_file ? "]" : ")");
                return;
            }
            else if (is_a_valid_function(current_symbol_type))
            {
                last_symbol_buffer_length = line_offset - 11;
            }
            line_offset = 0;
        }
        else
        {
            if (line_offset < 8)
            {
                uint32_t val = hex_char_to_int(ch);
                current_symbol_address &= ~((uint32_t)0xf << ((7 - line_offset) * 4));
                current_symbol_address |= val << ((7 - line_offset) * 4);
            }
            if (line_offset == 9)
            {
                current_symbol_type = ch;
                if (current_symbol_address != 0)    // && is_a_valid_function(current_symbol_type)
                {
                    last_symbol_address = symbol_address;
                    symbol_address = current_symbol_address;
                }
            }
            if (line_offset >= 11 && line_offset < 64 + 11) // && is_a_valid_function(current_symbol_type)
            {
                if (!(last_symbol_address <= eip && symbol_address > eip))
                {
                    last_symbol_buffer[line_offset - 11] = ch;
                    found_symbol_type = current_symbol_type;
                }
            }
            line_offset++;
        }
        file_offset++;
    }
}

void kernel_panic(interrupt_registers_t* registers)
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
        if (registers->cr2)
            printf("cr2:  0x%x (pde %u pte %u offset 0x%x)\n", registers->cr2, registers->cr2 >> 22, (registers->cr2 >> 12) & 0x3ff, registers->cr2 & 0xfff);
        else
        {
            printf("cr2 : ");
            tty_set_color(FG_LIGHTMAGENTA, BG_BLACK);
            printf("NULL\n");
            tty_set_color(FG_WHITE, BG_BLACK);
        }
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
            &((uint32_t*)((privilege_switch_interrupt_registers_t*)registers)->esp)[i];

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
    const int max_stack_frames = 10;

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
            printf("eip : 0x%x | ebp : 0x%x ", ebp->eip, ebp);
            LOG(DEBUG, "eip : 0x%x | ebp : 0x%x ", ebp->eip, ebp);
            print_kernel_symbol_name(ebp->eip - 1, (uint32_t)ebp);
            putchar('\n');
            ebp = (call_frame_t*)ebp->ebp;
        }
        i++;
    }
    #endif

    halt();
}