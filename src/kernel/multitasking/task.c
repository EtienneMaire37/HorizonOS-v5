#pragma once

void load_pd(void* ptr)
{
    load_pd_by_physaddr(virtual_address_to_physical((virtual_address_t)ptr));
}

void load_pd_by_physaddr(physical_address_t addr)
{
    if (addr >> 32)
    {
        LOG(CRITICAL, "Tried to load a page directory above 4GB");
        abort();
    }

    setting_cur_cr3 = true;
    current_cr3 = (uint32_t)addr;
    asm("mov cr3, eax" :: "a" (current_cr3));
    setting_cur_cr3 = false;
}

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

    LOG(DEBUG, "ELF header : ");
    LOG(DEBUG, "   Type : %u", header->type);
    LOG(DEBUG, "   Machine : %u", header->machine);
    LOG(DEBUG, "   Entry : 0x%x", header->entry);
    LOG(DEBUG, "   Program header offset : 0x%x", header->phoff);
    LOG(DEBUG, "   Section header offset : 0x%x", header->shoff);
    LOG(DEBUG, "   Flags : 0x%x", header->flags);

    _task->stack_phys = pfa_allocate_physical_page();
    if (ring == 3)
        _task->kernel_stack_phys = pfa_allocate_physical_page();
    else
        _task->kernel_stack_phys = 0;

    task_create_virtual_address_space(_task);

    // LOG(DEBUG, "Page directory data : ");
    // for (uint16_t i = 0; i < 1024; i++)
    // {
    //     LOG(DEBUG, "%u: 0x%x", i, read_physical_address_4b(_task->page_directory_phys + 4 * i));
    // }

    LOG(DEBUG, "Loading program cr3 0x%x", _task->page_directory_phys);
    load_pd_by_physaddr(_task->page_directory_phys);
    LOG(DEBUG, "Loaded program cr3");

    LOG(DEBUG, "ELF sections : ");

    struct elf32_program_header* program_headers = (struct elf32_program_header*)((uint32_t)header + header->phoff);
    struct elf32_section_header* section_headers = (struct elf32_section_header*)((uint32_t)header + header->shoff);
    struct elf32_section_header* string_table_header = &section_headers[header->shstrndx];
    char* string_table = (char*)((uint32_t)header + string_table_header->sh_offset);
    
    for (uint16_t i = 0; i < header->phnum; i++)
    {
        if (program_headers[i].type == ELF_PROGRAM_TYPE_NULL) continue;
        LOG(DEBUG, "   Program section %u : ", i);
        LOG(DEBUG, "       Type : %u", program_headers[i].type);
        LOG(DEBUG, "       Offset : 0x%x", program_headers[i].p_offset);
        LOG(DEBUG, "       Virtual address : 0x%x", program_headers[i].p_vaddr);
        LOG(DEBUG, "       Physical address : 0x%x", program_headers[i].p_paddr);
        LOG(DEBUG, "       File size : %u", program_headers[i].p_filesz);
        LOG(DEBUG, "       Memory size : %u", program_headers[i].p_memsz);
        LOG(DEBUG, "       Flags : 0x%x", program_headers[i].flags);
        LOG(DEBUG, "       Alignment : %u", program_headers[i].align);
    }

    for (uint16_t i = 0; i < header->shnum; i++)
    {
        if (section_headers[i].sh_type == 0) continue;
        LOG(DEBUG, "   Elf section %u : ", i);
        LOG(DEBUG, "       Name : %s", &string_table[section_headers[i].sh_name]);
        LOG(DEBUG, "       Type : %u", section_headers[i].sh_type);
        LOG(DEBUG, "       Flags : 0x%x", section_headers[i].sh_flags);
        LOG(DEBUG, "       Address : 0x%x", section_headers[i].sh_addr);
        LOG(DEBUG, "       Offset : 0x%x", section_headers[i].sh_offset);
        LOG(DEBUG, "       Size : %u", section_headers[i].sh_size);
        LOG(DEBUG, "       Link : %u", section_headers[i].sh_link);
        LOG(DEBUG, "       Info : %u", section_headers[i].sh_info);
        LOG(DEBUG, "       Address alignment : %u", section_headers[i].sh_addralign);
        LOG(DEBUG, "       Entry size : %u", section_headers[i].sh_entsize);

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
                LOG(DEBUG, "Allocating page at vaddr : 0x%x", address);
                task_virtual_address_space_create_page(_task, layout.page_directory_entry, layout.page_table_entry, ring == 3 ? PAGING_USER_LEVEL : PAGING_SUPERVISOR_LEVEL, 1);
                // memcpy((void*)paddr, (void*)((uint32_t)header + section_headers[i].sh_offset + j), 0x1000);
                memcpy((void*)address, (void*)((uint32_t)header + section_headers[i].sh_offset + j), 0x1000);
                // LOG(DEBUG, "Data at 0x%x (0x%x) : 0x%x 0x%x", vaddr + j, paddr, *(uint8_t*)paddr, *(uint8_t*)(paddr + 1));
                layout.page_offset += 0x1000;
                if (layout.page_offset == 0)
                {
                    layout.page_table_entry++;
                    if (layout.page_table_entry == 1024)
                    {
                        layout.page_table_entry = 0;
                        layout.page_directory_entry++;
                    }
                }
            }
        }
    }

    // uint8_t* task_stack_top = physical_address_to_virtual(_task->stack_phys + 4096) - sizeof(struct interrupt_registers); // (uint8_t*)TASK_STACK_TOP_ADDRESS;

    // // struct interrupt_registers* registers = ring == 3 ? (struct interrupt_registers*)((uint32_t)_task->kernel_stack + 4) : (struct interrupt_registers*)(task_stack_top - sizeof(struct interrupt_registers));
    // struct interrupt_registers* registers = (struct interrupt_registers*)(task_stack_top - sizeof(struct interrupt_registers));

    // registers->cs = ring == 3 ? USER_CODE_SEGMENT : KERNEL_CODE_SEGMENT;
    // registers->ds = ring == 3 ? USER_DATA_SEGMENT : KERNEL_DATA_SEGMENT;
    // registers->ss = ring == 3 ? USER_DATA_SEGMENT : KERNEL_DATA_SEGMENT;
    // registers->eflags = 0x200; // Interrupts enabled (bit 9)
    // registers->esp = (uint32_t)task_stack_top;
    // registers->handled_esp = (uint32_t)task_stack_top - 7 * 4; // (ring == 3 ? (uint32_t)_task->kernel_stack + KERNEL_STACK_SIZE : (uint32_t)task_stack_top) - 7 * 4;
    
    // registers->eax = registers->ebx = registers->ecx = registers->edx = 0;
    // registers->esi = registers->edi = registers->ebp = 0;

    // registers->eip = header->entry;

    // _task->registers = registers;

    struct interrupt_registers registers;

    registers.cs = ring == 3 ? USER_CODE_SEGMENT : KERNEL_CODE_SEGMENT;
    registers.ds = ring == 3 ? USER_DATA_SEGMENT : KERNEL_DATA_SEGMENT;
    registers.ss = ring == 3 ? USER_DATA_SEGMENT : KERNEL_DATA_SEGMENT;
    registers.eflags = 0x200; // Interrupts enabled (bit 9)
    registers.esp = TASK_STACK_TOP_ADDRESS;
    registers.handled_esp = (uint32_t)TASK_STACK_TOP_ADDRESS - 7 * 4; // (ring == 3 ? (uint32_t)_task->kernel_stack + KERNEL_STACK_SIZE : (uint32_t)task_stack_top) - 7 * 4;
    
    registers.eax = registers.ebx = registers.ecx = registers.edx = 0;
    registers.esi = registers.edi = registers.ebp = 0;

    registers.eip = header->entry;

    for (uint32_t i = 0; i < sizeof(struct interrupt_registers); i++)
        write_physical_address_1b((_task->stack_phys + 4096) - sizeof(struct interrupt_registers), *((uint8_t*)&registers + i));

    // *(uint32_t*)&_task->kernel_stack[0] = (uint32_t)registers;

    // size_t name_length = kstrlen(name);
    // if (name_length >= 31)
    // {
    //     memcpy(_task->name, name, 31);
    //     _task->name[31] = '\0';
    // }
    // else
    // {
    //     memcpy(_task->name, name, name_length);
    //     _task->name[name_length] = '\0';
    // }

    _task->name = name;

    _task->ring = ring;

    LOG(DEBUG, "Successfully loaded task \"%s\" from initrd", name);
}

void task_destroy(struct task* _task)
{
    LOG(INFO, "Destroying task \"%s\" (pid = %lu, ring = %u)", _task->name, _task->pid, _task->ring);
    LOG(TRACE, "Freeing stack at 0x%x", _task->stack_phys); //_task->stack);
    // pfa_free_page((virtual_address_t)_task->stack);
    if (_task->stack_phys)
        pfa_free_physical_page(_task->stack_phys);
    LOG(TRACE, "Freeing virtual address space");
    task_virtual_address_space_destroy(_task);
    // if (_task->kernel_stack)
    if (_task->kernel_stack_phys)
        pfa_free_physical_page(_task->kernel_stack_phys);
        // pfa_free_page((virtual_address_t)_task->kernel_stack);
}

void task_virtual_address_space_destroy(struct task* _task)
{
    for (uint16_t i = 0; i < 1024; i++)
    {
        if (read_physical_address_4b(_task->page_directory_phys + 4 * i) & 1)
        {
            physical_address_t pt_paddr = read_physical_address_4b(_task->page_directory_phys + 4 * i) & 0xfffff000;

            for (uint16_t j = 0; j < 1024; j++)
            {
                if ((read_physical_address_4b(pt_paddr + 4 * j) & 1) && !(i == 0 && j < 256) && i < 768)
                {
                    pfa_free_physical_page(read_physical_address_4b(pt_paddr + 4 * j) & 0xfffff000);
                }
            }
            // LOG(TRACE, "Freeing page table at 0x%x", pt);
            pfa_free_physical_page(pt_paddr);
        }
    }
    pfa_free_physical_page(_task->page_directory_phys);
}

void task_virtual_address_space_create_page_table(struct task* _task, uint16_t index)
{
    physical_address_t pt_phys = pfa_allocate_physical_page();

    // LOG(DEBUG, "pd: 0x%lx, pt: 0x%lx", _task->page_directory_phys, pt_phys);

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
    if (!(read_physical_address_4b(_task->page_directory_phys + 4 * pd_index) & 1))
        task_virtual_address_space_create_page_table(_task, pd_index);

    physical_address_t pt_phys = read_physical_address_4b(_task->page_directory_phys + 4 * pd_index) & 0xfffff000;

    physical_address_t page = pfa_allocate_physical_page();
    physical_set_page(pt_phys, pt_index, page, user_supervisor, read_write);
    return page;
}

void task_create_virtual_address_space(struct task* _task)
{
    _task->page_directory_phys = pfa_allocate_physical_page();

    physical_init_page_directory(_task->page_directory_phys);

    _task->registers->cr3 = _task->page_directory_phys;

    physical_add_page_table(_task->page_directory_phys, 1023, _task->page_directory_phys, PAGING_SUPERVISOR_LEVEL, true);

    task_virtual_address_space_create_page_table(_task, 0);
    physical_address_t first_mb_pt_address = read_physical_address_4b(_task->page_directory_phys) & 0xfffff000;
    for (uint16_t i = 0; i < 256; i++)
        physical_set_page(first_mb_pt_address, i, i * 0x1000, PAGING_SUPERVISOR_LEVEL, true);

    for (uint16_t i = 0; i < 255; i++)  // !!!!! DONT OVERRIDE RECURSIVE PAGING I SPENT WAY TO MUCH TIME DEBUGGING THIS
    {
        task_virtual_address_space_create_page_table(_task, i + 768);
        for (uint16_t j = 0; j < 1024; j++)
        {
            struct virtual_address_layout layout;
            layout.page_directory_entry = i + 768;
            layout.page_table_entry = j;
            layout.page_offset = 0;
            uint32_t address = *(uint32_t*)&layout - 0xc0000000;
            physical_address_t pt_address = read_physical_address_4b(_task->page_directory_phys + 4 * layout.page_directory_entry) & 0xfffff000;
            physical_set_page(pt_address, layout.page_table_entry, address, PAGING_SUPERVISOR_LEVEL, true);
            // LOG(DEBUG, "Mapped 0x%x-0x%x to 0x%x-0x%x", *(uint32_t*)&layout, *(uint32_t*)&layout + 4096, address, address + 4096);
        }
    }

    LOG(DEBUG, "Page directory data 1 : ");
    for (uint16_t i = 0; i < 1024; i++)
    {
        LOG(DEBUG, "%u: 0x%x", i, read_physical_address_4b(_task->page_directory_phys + 4 * i));
    }

    task_virtual_address_space_create_page_table(_task, 767);   // Stacks and physical memory access    // !!!!!!!!!!! This line seems to mess up everything
    
    LOG(DEBUG, "Page directory data 2 : ");
    for (uint16_t i = 0; i < 1024; i++)
    {
        LOG(DEBUG, "%u: 0x%x", i, read_physical_address_4b(_task->page_directory_phys + 4 * i));
    }

    physical_address_t pt_address = read_physical_address_4b(_task->page_directory_phys + 4 * 767) & 0xfffff000;
    physical_set_page(pt_address, 1023, _task->stack_phys, PAGING_USER_LEVEL, true);
    physical_set_page(pt_address, 1022, _task->kernel_stack_phys, PAGING_USER_LEVEL, true);
    physical_set_page(pt_address, 1021, 0, PAGING_USER_LEVEL, true);

    // LOG(DEBUG, "Page directory data 3 : ");
    // for (uint16_t i = 0; i < 1024; i++)
    // {
    //     LOG(DEBUG, "%u: 0x%x", i, read_physical_address_4b(_task->page_directory_phys + 4 * i));
    // }
}

void multitasking_init()
{
    task_count = 0;
    current_task_index = 0;
    current_pid = 0;
}

void multitasking_start()
{
    multitasking_enabled = true;
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
    tasks[task_count].pid = current_pid++;
    tasks[task_count].system_task = system;
    task_count++;
}

void switch_task(struct interrupt_registers** registers)
{
    if (task_count == 0)
    {
        LOG(CRITICAL, "No task to switch to");
        abort();
    }

    if (!first_task_switch) 
        tasks[current_task_index].registers = *registers;
    else
        current_task_index = task_count - 1;
    first_task_switch = false;

    current_task_index = (current_task_index + 1) % task_count;

    TSS.esp0 = TASK_KERNEL_STACK_TOP_ADDRESS; //(uint32_t)tasks[current_task_index].kernel_stack + 4096;
    TSS.ss0 = KERNEL_DATA_SEGMENT;

    *registers = tasks[current_task_index].registers;

    LOG(DEBUG, "Switched to task \"%s\" (pid = %lu, ring = %u) | registers : esp : 0x%x, 0x%x : end esp : 0x%x | eip : 0x%x, cs : 0x%x, eflags : 0x%x, ss : 0x%x, cr3 : 0x%x, ds : 0x%x, eax : 0x%x, ebx : 0x%x, ecx : 0x%x, edx : 0x%x, esi : 0x%x, edi : 0x%x", 
        tasks[current_task_index].name, tasks[current_task_index].pid, tasks[current_task_index].ring, 
        tasks[current_task_index].registers->esp, tasks[current_task_index].registers->handled_esp, *registers, 
        tasks[current_task_index].registers->eip, tasks[current_task_index].registers->cs, tasks[current_task_index].registers->eflags, tasks[current_task_index].registers->ss, tasks[current_task_index].registers->cr3, tasks[current_task_index].registers->ds,
        tasks[current_task_index].registers->eax, tasks[current_task_index].registers->ebx, tasks[current_task_index].registers->ecx, tasks[current_task_index].registers->edx, tasks[current_task_index].registers->esi, tasks[current_task_index].registers->edi);
}

void task_kill(uint16_t index)
{
    if (index >= task_count)
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