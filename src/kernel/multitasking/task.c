#pragma once

void task_load_from_initrd(struct task* _task, char* name, uint8_t ring)
{
    if (ring != 3 && ring != 0)
    {
        LOG(CRITICAL, "Invalid ring level");
        abort();
    }

    LOG(INFO, "Loading task \"%s\" from initrd", name);

    struct initrd_file* file = initrd_find_file(name);
    if (!file) 
    {
        LOG(CRITICAL, "File \"%s\" not found in initrd", name);
        abort();
    }

    struct elf32_header* header = (struct elf32_header*)file->data;

    if (memcmp(header->magic, "\x7f""ELF", 4) != 0)
    {
        LOG(CRITICAL, "Invalid ELF header");
        abort();
    }

    if (header->architecture != ELF_CLASS_32)
    {
        LOG(CRITICAL, "Invalid ELF architecture");
        abort();
    }

    if (header->byte_order != ELF_DATA_LITTLE_ENDIAN)
    {
        LOG(CRITICAL, "Invalid ELF byte order");
        abort();
    }

    if (header->osabi != ELF_OSABI_SYSV)
    {
        LOG(CRITICAL, "Invalid ELF OSABI");
        abort();
    }

    if (header->machine != ELF_INSTRUCTION_SET_x86)
    {
        LOG(CRITICAL, "Invalid ELF machine");
        abort();
    }

    if (header->type != ELF_TYPE_EXECUTABLE)
    {
        LOG(CRITICAL, "Invalid ELF type");
        abort();
    }

    LOG(TRACE, "ELF header : ");
    LOG(TRACE, "   Type : %u", header->type);
    LOG(TRACE, "   Machine : %u", header->machine);
    LOG(TRACE, "   Entry : 0x%x", header->entry);
    LOG(TRACE, "   Program header offset : 0x%x", header->phoff);
    LOG(TRACE, "   Section header offset : 0x%x", header->shoff);
    LOG(TRACE, "   Flags : 0x%x", header->flags);

    utf32_buffer_init(&_task->input_buffer);
    _task->kernel_thread = false;
    _task->reading_stdin = _task->was_reading_stdin = false;
    _task->stack_phys = pfa_allocate_physical_page();
    if (ring != 0)
        _task->kernel_stack_phys = pfa_allocate_physical_page();

    LOG(TRACE, "Stack physical addres: 0x%lx", _task->stack_phys);
    LOG(TRACE, "Kernel stack physical addres: 0x%lx", _task->kernel_stack_phys);

    task_create_virtual_address_space(_task);

    struct elf32_program_header* program_headers = (struct elf32_program_header*)((uint32_t)header + header->phoff);
    struct elf32_section_header* section_headers = (struct elf32_section_header*)((uint32_t)header + header->shoff);
    struct elf32_section_header* string_table_header = &section_headers[header->shstrndx];
    char* string_table = (char*)((uint32_t)header + string_table_header->sh_offset);
    
    for (uint16_t i = 0; i < header->phnum; i++)
    {
        if (program_headers[i].type == ELF_PROGRAM_TYPE_NULL) continue;

        LOG(TRACE, "   Program section %u : ", i);
        LOG(TRACE, "       Type : %u", program_headers[i].type);
        LOG(TRACE, "       Offset : 0x%x", program_headers[i].p_offset);
        LOG(TRACE, "       Virtual address : 0x%x", program_headers[i].p_vaddr);
        LOG(TRACE, "       Physical address : 0x%x", program_headers[i].p_paddr);
        LOG(TRACE, "       File size : %u", program_headers[i].p_filesz);
        LOG(TRACE, "       Memory size : %u", program_headers[i].p_memsz);
        LOG(TRACE, "       Flags : 0x%x", program_headers[i].flags);
        LOG(TRACE, "       Alignment : %u", program_headers[i].align);
    }

    for (uint16_t i = 0; i < header->shnum; i++)
    {
        if (section_headers[i].sh_type == 0) continue;

        LOG(TRACE, "   Elf section %u : ", i);
        LOG(TRACE, "       Name : %s", &string_table[section_headers[i].sh_name]);
        LOG(TRACE, "       Type : %u", section_headers[i].sh_type);
        LOG(TRACE, "       Flags : 0x%x", section_headers[i].sh_flags);
        LOG(TRACE, "       Address : 0x%x", section_headers[i].sh_addr);
        LOG(TRACE, "       Offset : 0x%x", section_headers[i].sh_offset);
        LOG(TRACE, "       Size : %u", section_headers[i].sh_size);
        LOG(TRACE, "       Link : %u", section_headers[i].sh_link);
        LOG(TRACE, "       Info : %u", section_headers[i].sh_info);
        LOG(TRACE, "       Address alignment : %u", section_headers[i].sh_addralign);
        LOG(TRACE, "       Entry size : %u", section_headers[i].sh_entsize);

        if (section_headers[i].sh_flags & ELF_SECTION_FLAG_ALLOC)   // Allocate section
        {
            virtual_address_t vaddr = section_headers[i].sh_addr;
            if (vaddr & 0xfff)
            {
                LOG(CRITICAL, "Section is not page aligned");
                abort();
            }
            for (uint32_t j = 0; j < section_headers[i].sh_size; j += 0x1000) 
            {
                uint32_t address = vaddr + j;
                struct virtual_address_layout layout = *(struct virtual_address_layout*)&address;
            
                task_virtual_address_space_create_page(_task, layout.page_directory_entry, layout.page_table_entry, ring == 3 ? PAGING_USER_LEVEL : PAGING_SUPERVISOR_LEVEL, 1);
                physical_address_t pt_phys = read_physical_address_4b(_task->page_directory_phys + 4 * layout.page_directory_entry) & 0xfffff000;
                
                if (section_headers[i].sh_type == ELF_SECTION_TYPE_PROGBITS)
                {
                    set_current_phys_mem_page(read_physical_address_4b(pt_phys + 4 * layout.page_table_entry) >> 12);
                    memcpy((void*)PHYS_MEM_PAGE_BOTTOM, (void*)((uint32_t)header + section_headers[i].sh_offset + j), min(0x1000, section_headers[i].sh_size - j));
                }
                else
                {
                    // ~ Assumes page aligned sections
                    memset_page(read_physical_address_4b(pt_phys + 4 * layout.page_table_entry) & 0xfffff000, 0);
                }
            }
        }
    }

    fpu_state_init(&(_task->fpu_state));

    struct privilege_switch_interrupt_registers registers;

    registers.cs = ring == 3 ? USER_CODE_SEGMENT : KERNEL_CODE_SEGMENT;
    registers.ds = ring == 3 ? USER_DATA_SEGMENT : KERNEL_DATA_SEGMENT;
    registers.ss = registers.ds;

    registers.eflags = 0x200; // Interrupts enabled (bit 9)
    registers.ebp = 0;
    registers.esp = TASK_STACK_TOP_ADDRESS;
    registers.handled_esp = registers.esp - 0x2c - 8;   // 8 for the user's ss and esp

    _task->registers_ptr = (struct privilege_switch_interrupt_registers*)(TASK_STACK_TOP_ADDRESS - sizeof(struct privilege_switch_interrupt_registers));
    
    registers.eax = registers.ebx = registers.ecx = registers.edx = 0;
    registers.esi = registers.edi = 0;

    registers.eip = header->entry;

    registers.cr3 = _task->page_directory_phys;

    _task->registers_data = registers;

    _task->name = name;

    _task->ring = ring;

    for (uint16_t i = 0; i < sizeof(registers); i++)
        write_physical_address_1b((physical_address_t)((uint32_t)_task->registers_ptr) + _task->stack_phys - TASK_STACK_BOTTOM_ADDRESS + i,
            ((uint8_t*)&_task->registers_data)[i]);

    LOG(DEBUG, "Successfully loaded task \"%s\" from initrd", name);
}

void task_destroy(struct task* _task)
{
    LOG(DEBUG, "Destroying task \"%s\" (pid = %lu, ring = %u)", _task->name, _task->pid, _task->ring);
    LOG(TRACE, "Freeing virtual address space");
    task_virtual_address_space_destroy(_task);
    if (_task->kernel_stack_phys)
    {
        LOG(TRACE, "Freeing kernel stack at 0x%x", _task->kernel_stack_phys);
        pfa_free_physical_page(_task->kernel_stack_phys);
    }
    if (_task->stack_phys)
    {
        LOG(TRACE, "Freeing stack at 0x%x", _task->stack_phys);
        pfa_free_physical_page(_task->stack_phys);
    }
    LOG(TRACE, "Freeing input buffer");
    utf32_buffer_destroy(&_task->input_buffer);
}

void task_virtual_address_space_destroy(struct task* _task)
{
    for (uint16_t i = 0; i < 768; i++)
    {
        uint32_t pde = read_physical_address_4b(_task->page_directory_phys + 4 * i);
        if (pde & 1)
        {
            physical_address_t pt_paddr = pde & 0xfffff000;

            for (uint16_t j = (i == 0 ? 256 : 0); j < (i == 767 ? 1021 : 1024); j++)
            {
                uint32_t pte = read_physical_address_4b(pt_paddr + 4 * j);

                if (pte & 1)
                    pfa_free_physical_page(pte & 0xfffff000);
            }
            pfa_free_physical_page(pt_paddr);
        }
    }
    pfa_free_physical_page(_task->page_directory_phys);
}

void task_virtual_address_space_create_page_table(struct task* _task, uint16_t index)
{
    physical_address_t pt_phys = pfa_allocate_physical_page();

    physical_init_page_table(pt_phys);

    physical_add_page_table(_task->page_directory_phys, index, pt_phys, PAGING_USER_LEVEL, true);
}

void task_virtual_address_space_remove_page_table(struct task* _task, uint16_t index)
{
    physical_address_t pt_phys = read_physical_address_4b(_task->page_directory_phys + 4 * index) & 0xfffff000;

    for (uint16_t i = 0; i < 1024; i++)
    {
        if (read_physical_address_4b(pt_phys + 4 * i) & 1)
            pfa_free_physical_page(read_physical_address_4b(pt_phys + 4 * i) & 0xfffff000);
    }

    pfa_free_physical_page(pt_phys);

    physical_remove_page_table(_task->page_directory_phys, index);
}

physical_address_t task_virtual_address_space_create_page(struct task* _task, uint16_t pd_index, uint16_t pt_index, uint8_t user_supervisor, uint8_t read_write)
{
    uint32_t pde = read_physical_address_4b(_task->page_directory_phys + 4 * pd_index);

    if (!(pde & 1))
        task_virtual_address_space_create_page_table(_task, pd_index);

    physical_address_t pt_phys = pde & 0xfffff000;

    physical_address_t page = pfa_allocate_physical_page();
    physical_set_page(pt_phys, pt_index, page, user_supervisor, read_write);
    return page;
}

void task_create_virtual_address_space(struct task* _task)
{
    LOG(DEBUG, "Creating a new virtual address space...");

    _task->page_directory_phys = pfa_allocate_physical_page();

    physical_init_page_directory(_task->page_directory_phys);

    physical_add_page_table(_task->page_directory_phys, 1023, _task->page_directory_phys, PAGING_SUPERVISOR_LEVEL, true);

    _task->registers_data.cr3 = _task->page_directory_phys;

    task_virtual_address_space_create_page_table(_task, 0);
    physical_address_t first_mb_pt_address = read_physical_address_4b(_task->page_directory_phys) & 0xfffff000;
    for (uint16_t i = 0; i < 256; i++)
        physical_set_page(first_mb_pt_address, i, i * 0x1000, PAGING_SUPERVISOR_LEVEL, true);

    for (uint16_t i = 0; i < 255; i++)  // !!!!! DONT OVERRIDE RECURSIVE PAGING
    {
        task_virtual_address_space_create_page_table(_task, i + 768);
        struct virtual_address_layout layout;
        layout.page_directory_entry = i + 768;
        layout.page_offset = 0;
        physical_address_t pt_address = read_physical_address_4b(_task->page_directory_phys + 4 * layout.page_directory_entry) & 0xfffff000;
        for (uint16_t j = 0; j < 1024; j++)
        {
            layout.page_table_entry = j;
            uint32_t address = *(uint32_t*)&layout - 0xc0000000;
            physical_set_page(pt_address, layout.page_table_entry, address, PAGING_SUPERVISOR_LEVEL, true);
        }
    }

    task_virtual_address_space_create_page_table(_task, 767);   // Stacks and physical memory access

    physical_address_t pt_address = read_physical_address_4b(_task->page_directory_phys + 4 * 767) & 0xfffff000;
    physical_set_page(pt_address, 1021, 0, PAGING_SUPERVISOR_LEVEL, true);
    physical_set_page(pt_address, 1022, _task->kernel_stack_phys, PAGING_SUPERVISOR_LEVEL, true);
    physical_set_page(pt_address, 1023, _task->stack_phys, PAGING_USER_LEVEL, true);

    LOG(DEBUG, "Done.");
}

void multitasking_init()
{
    task_count = 0;
    current_task_index = 0;
    current_pid = 0;
    zombie_task_index = 0;

    fpu_state_init(&(tasks[task_count].fpu_state));

    tasks[task_count].kernel_thread = true;
    tasks[task_count].pid = current_pid++;
    tasks[task_count].system_task = true;
    tasks[task_count].ring = 0;
    tasks[task_count].name = "idle";
    tasks[task_count].reading_stdin = tasks[task_count].was_reading_stdin = false;
    utf32_buffer_init(&tasks[task_count].input_buffer);

    tasks[task_count].stack_phys = pfa_allocate_physical_page();
    tasks[task_count].kernel_stack_phys = pfa_allocate_physical_page();

    task_create_virtual_address_space(&tasks[task_count]);

    tasks[task_count].registers_data.eip = (uint32_t)&idle_main;
    tasks[task_count].registers_data.cs = KERNEL_CODE_SEGMENT;
    tasks[task_count].registers_data.ds = KERNEL_DATA_SEGMENT;

    tasks[task_count].registers_data.eflags = 0x200;
    tasks[task_count].registers_data.ebp = 0;

    tasks[task_count].registers_ptr = (struct privilege_switch_interrupt_registers*)(TASK_STACK_TOP_ADDRESS - sizeof(struct interrupt_registers));
    tasks[task_count].registers_data.handled_esp = TASK_STACK_TOP_ADDRESS - 0x2c;
    
    tasks[task_count].registers_data.eax = tasks[task_count].registers_data.ebx = tasks[task_count].registers_data.ecx = tasks[task_count].registers_data.edx = 0;
    tasks[task_count].registers_data.esi = tasks[task_count].registers_data.edi = 0;

    for (uint16_t i = 0; i < sizeof(struct interrupt_registers); i++)
        write_physical_address_1b((physical_address_t)((uint32_t)tasks[task_count].registers_ptr) + tasks[task_count].stack_phys - TASK_STACK_BOTTOM_ADDRESS + i,
            ((uint8_t*)&tasks[task_count].registers_data)[i]);

    task_count++;
}

void multitasking_start()
{
    fflush(stdout);
    multitasking_enabled = true;
    current_task_index = task_count - 1;    // ~ ((task_count - 1) + 1) % task_count = 0 - So it starts with the idle task
    while(true);
}

void multasking_add_task_from_initrd(char* path, uint8_t ring, bool system)
{
    if (task_count >= MAX_TASKS)
    {
        LOG(CRITICAL, "Too many tasks");
        abort();
    }

    task_load_from_initrd(&tasks[task_count], path, ring);
    tasks[task_count].pid  = current_pid++;
    tasks[task_count].system_task = system;
    task_count++;
}

void task_write_at_address_1b(struct task* _task, uint32_t address, uint8_t value)
{
    struct virtual_address_layout layout = *(struct virtual_address_layout*)&address;
    uint32_t pde = read_physical_address_4b(_task->page_directory_phys + 4 * layout.page_directory_entry);
    if (!(pde & 1)) return;
    physical_address_t pt_address = (physical_address_t)pde & 0xfffff000;
    uint32_t pte = read_physical_address_4b(pt_address + 4 * layout.page_table_entry);
    if (!(pte & 1)) return;
    physical_address_t page_address = (physical_address_t)pte & 0xfffff000;
    physical_address_t byte_address = page_address | layout.page_offset;
    write_physical_address_1b(byte_address, value);
}

void switch_task(struct privilege_switch_interrupt_registers** registers, bool* flush_tlb, uint32_t* iret_cr3)
{
    if (task_count == 0)
    {
        LOG(CRITICAL, "No task to switch to");
        abort();
    }

    if (!first_task_switch)
    {
        tasks[current_task_index].registers_data = **registers;
        tasks[current_task_index].registers_ptr = *registers;

        fpu_save_state(&tasks[current_task_index].fpu_state);
    }

    uint16_t next_task_index = find_next_task_index();
    if (next_task_index == current_task_index && !first_task_switch)
        return;
    current_task_index = next_task_index;

    LOG(TRACE, "Switched to task \"%s\" (pid = %lu, ring = %u) | registers : esp : 0x%x : end esp : 0x%x | ebp : 0x%x | eip : 0x%x, cs : 0x%x, eflags : 0x%x, ds : 0x%x, eax : 0x%x, ebx : 0x%x, ecx : 0x%x, edx : 0x%x, esi : 0x%x, edi : 0x%x, cr3 : 0x%x", 
        tasks[current_task_index].name, tasks[current_task_index].pid, tasks[current_task_index].ring, 
        tasks[current_task_index].registers_data.handled_esp, tasks[current_task_index].registers_ptr, tasks[current_task_index].registers_data.ebp,
        tasks[current_task_index].registers_data.eip, tasks[current_task_index].registers_data.cs, tasks[current_task_index].registers_data.eflags, tasks[current_task_index].registers_data.ds,
        tasks[current_task_index].registers_data.eax, tasks[current_task_index].registers_data.ebx, tasks[current_task_index].registers_data.ecx, tasks[current_task_index].registers_data.edx, tasks[current_task_index].registers_data.esi, tasks[current_task_index].registers_data.edi,
        tasks[current_task_index].registers_data.cr3);

    *flush_tlb = true;

    if (tasks[current_task_index].ring == 3)
        TSS.esp0 = TASK_KERNEL_STACK_TOP_ADDRESS;

    *iret_cr3 = tasks[current_task_index].registers_data.cr3;
    *registers = tasks[current_task_index].registers_ptr;       // * When poping esp load the correct values

    if (tasks[current_task_index].was_reading_stdin)
    {
        uint32_t _eax = min(get_buffered_characters(tasks[current_task_index].input_buffer), tasks[current_task_index].registers_data.edx);
        task_write_register_data(&tasks[current_task_index], eax, _eax);
        for (uint32_t i = 0; i < _eax; i++)
        {
            // *** Only ASCII for now ***
            task_write_at_address_1b(&tasks[current_task_index], tasks[current_task_index].registers_data.ecx + i, utf32_to_bios_oem(utf32_buffer_getchar(&tasks[current_task_index].input_buffer)));
        }
    }

    tasks[current_task_index].was_reading_stdin = false;

    fpu_restore_state(&tasks[current_task_index].fpu_state);

    first_task_switch = false;
}

void task_kill(uint16_t index)
{
    if (index >= task_count || index == 0 || index == current_task_index)
    {
        LOG(CRITICAL, "Invalid task index %u", index);
        abort();
    }
    if (task_count == 1)
    {
        LOG(CRITICAL, "Zero tasks remaining");
        abort();
    }

    task_destroy(&tasks[index]);
    for (uint16_t i = index; i < task_count - 1; i++)
        tasks[i] = tasks[i + 1];
    if (current_task_index >= index)
        current_task_index--;
    task_count--;
}

void idle_main()
{
    while(true) hlt();
}