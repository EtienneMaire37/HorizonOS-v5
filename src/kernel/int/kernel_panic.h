#pragma once

#include "../multitasking/task.h"
#include "../cpu/memory.h"

#define is_a_valid_function(symbol_type) ((symbol_type) == 'T' || (symbol_type) == 'R' || (symbol_type) == 't' || (symbol_type) == 'r')  

void print_kernel_symbol_name(uintptr_t rip, uintptr_t rbp)
{
    initrd_file_t* file = kernel_symbols_file;
    if (file == NULL) return;
    if (file->size == 0 || file->data == NULL) return;
    
    uintptr_t symbol_address = 0, last_symbol_address = 0, current_symbol_address = 0;
    uintptr_t file_offset = 0, line_offset = 0;
    char last_symbol_buffer[64] = {0};
    uint16_t last_symbol_buffer_length = 0;
    char current_symbol_type = ' ', found_symbol_type = ' ';
    while (file_offset < file->size)
    {
        char ch = file->data[file_offset];
        if (ch == '\n')
        {
            if (last_symbol_address <= rip && symbol_address > rip && is_a_valid_function(found_symbol_type))
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
                last_symbol_buffer_length = line_offset - 19;
            }
            line_offset = 0;
        }
        else
        {
            if (line_offset < 16)
            {
                uint64_t val = hex_char_to_int(ch);
                current_symbol_address &= ~((uint64_t)0xf << ((15 - line_offset) * 4));
                current_symbol_address |= val << ((15 - line_offset) * 4);
            }
            if (line_offset == 17)
            {
                current_symbol_type = ch;
                if (current_symbol_address != 0)    // && is_a_valid_function(current_symbol_type)
                {
                    last_symbol_address = symbol_address;
                    symbol_address = current_symbol_address;
                }
            }
            if (line_offset >= 19 && line_offset < 64 + 19) // && is_a_valid_function(current_symbol_type)
            {
                if (!(last_symbol_address <= rip && symbol_address > rip))
                {
                    last_symbol_buffer[line_offset - 19] = ch;
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

    tty_update_cursor();
    // tty_hide_cursor();

    tty_set_color(FG_LIGHTRED, BG_BLACK);
    printf("Kernel panic\n\n");

    tty_set_color(FG_LIGHTGREEN, BG_BLACK);

    if (multitasking_enabled)
        printf("Task : \"%s\" (pid = %d) | ring = %u\n\n", tasks[current_task_index].name, tasks[current_task_index].pid, tasks[current_task_index].ring);

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
        {
            printf("cr2:  0x%x (pml4e %u pdpte %u pde %u pte %u offset 0x%x)\n", registers->cr2, (registers->cr2 >> 39) & 0x1ff, (registers->cr2 >> 30) & 0x1ff, (registers->cr2 >> 21) & 0x1ff, (registers->cr2 >> 12) & 0x1ff, registers->cr2 & 0xfff);
        }
        else
        {
            printf("cr2 : ");
            tty_set_color(FG_LIGHTMAGENTA, BG_BLACK);
            printf("NULL\n");
            tty_set_color(FG_WHITE, BG_BLACK);
        }
        printf("cr3: 0x%x\n\n", registers->cr3);

        // uint32_t pde = read_physical_address_4b(registers->cr3 + 4 * (registers->cr2 >> 22));
        
        // LOG(DEBUG, "Page directory entry : 0x%x", pde);

        // if (pde & 1)
        // {
        //     LOG(DEBUG, "Page table entry : 0x%x", read_physical_address_4b((pde & 0xfffff000) + 4 * ((registers->cr2 >> 12) & 0x3ff)));
        // }
    }

    LOG(DEBUG, "RSP=0x%x RBP=0x%x RAX=0x%x RBX=0x%x RCX=0x%x RDX=0x%x",
    registers->rsp, registers->rbp, registers->rax, registers->rbx, registers->rcx, registers->rdx);
    LOG(DEBUG, "R8=0x%x R9=0x%x R10=0x%x R11=0x%x R12=0x%x R13=0x%x R14=0x%x R15=0x%x",
    registers->r8, registers->r9, registers->r10, registers->r11, registers->r12, registers->r13, registers->r14, registers->r15);

    printf("RSP=0x%x RBP=0x%x\n",
    registers->rsp, registers->rbp);
    printf("RAX=0x%x RBX=0x%x RCX=0x%x RDX=0x%x\n", registers->rax, registers->rbx, registers->rcx, registers->rdx);
    printf("R8=0x%x R9=0x%x R10=0x%x R11=0x%x\n",
    registers->r8, registers->r9, registers->r10, registers->r11);
    printf("R12=0x%x R13=0x%x R14=0x%x R15=0x%x\n\n", registers->r12, registers->r13, registers->r14, registers->r15);

    printf("Stack trace : \n");
    LOG(DEBUG, "Stack trace : ");

    #define TRACE_FUNCTIONS
    #ifndef TRACE_FUNCTIONS
    for (int i = 0; i < 8; i++)
    {
        uint32_t* ptr = !(tasks[current_task_index].ring != 0 && multitasking_enabled && !first_task_switch) ? 
            &((uint32_t*)&registers[1])[i] : 
            &((uint32_t*)((privilege_switch_interrupt_registers_t*)registers)->rsp)[i];

        printf("rsp + %u (0x%x) : 0x%x\n", i * 4, ptr, *ptr);
    }
    #else
    typedef struct __attribute__((packed)) call_frame
    {
        uintptr_t rbp;
        uintptr_t rip;
    } call_frame_t;

    call_frame_t* rbp = (call_frame_t*)registers->rbp;
    // asm volatile ("mov eax, rbp" : "=a"(rbp));

    // ~ Log the last function (the one the exception happened in)
    printf("rip : 0x");
    tty_set_color(FG_YELLOW, BG_BLACK);
    printf("%x", registers->rip);
    tty_set_color(FG_WHITE, BG_BLACK);
    putchar(' ');

    LOG(DEBUG, "rip : 0x%x ", registers->rip);
    print_kernel_symbol_name(registers->rip, (uintptr_t)rbp);
    putchar('\n');

    int i = 0;
    const int max_stack_frames = 10;

    while (i <= max_stack_frames && is_address_canonical((uintptr_t)rbp) && is_address_canonical((uintptr_t)rbp->rip) && (uintptr_t)rbp != 0 && (uintptr_t)rbp->rip != 0 && 
    (((!multitasking_enabled || (multitasking_enabled && first_task_switch)) &&
            (((uintptr_t)rbp > 0xffffffffffffffff - 1024 * bootboot.numcores) && ((uintptr_t)rbp <= 0xffffffffffffffff))) || 
        (((uintptr_t)rbp < TASK_STACK_TOP_ADDRESS) && ((uintptr_t)rbp >= TASK_STACK_BOTTOM_ADDRESS))))
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
            printf("rip : 0x%x | rbp : 0x%x ", rbp->rip, rbp);
            LOG(DEBUG, "rip : 0x%x | rbp : 0x%x ", rbp->rip, rbp);
            print_kernel_symbol_name(rbp->rip - 1, (uintptr_t)rbp);
            putchar('\n');
            rbp = (call_frame_t*)rbp->rbp;
        }
        i++;
    }
    #endif

    // LOG(DEBUG, "VAS:");
    // LOG(DEBUG, "...");
    // for (int i = 0; i < 768; i++)
    // {
    //     uint32_t pde = read_physical_address_4b(registers->cr3 + 4 * i);
    //     if (!(pde & 1)) 
    //         continue;
    //     for (int j = (i == 0 ? 256 : 0); j < 1024; j++)
    //     {
    //         uint32_t pte = read_physical_address_4b((pde & 0xfffff000) + 4 * j);
    //         if (pte & 1)
    //         {
    //             uint32_t address = pte & 0xfffff000;
    //             LOG(DEBUG, "0x%x - 0x%x", address, address + 0x1000);
    //         }
    //     }
    // }
    // LOG(DEBUG, "...");

    halt();
}