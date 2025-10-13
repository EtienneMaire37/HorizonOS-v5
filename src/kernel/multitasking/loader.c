#pragma once

void multitasking_add_task_from_function(char* name, void (*func)())
{
    LOG(DEBUG, "Adding task \"%s\" from function", name);

    thread_t task = task_create_empty();
    // task.name = name;
    int name_bytes = minint(strlen(name), THREAD_NAME_MAX - 1);
    memcpy(task.name, name, name_bytes);
    task.name[name_bytes] = 0;
    task.cr3 = vas_create_empty();

    task.esp = TASK_STACK_TOP_ADDRESS;

    task_stack_push(&task, 0x200);
    task_stack_push(&task, KERNEL_CODE_SEGMENT);
    task_stack_push(&task, (uint32_t)func);

    task_stack_push(&task, (uint32_t)iret_instruction);

    task_stack_push(&task, 0);      // ebx
    task_stack_push(&task, 0);      // esi
    task_stack_push(&task, 0);      // edi
    task_stack_push(&task, 0);      // ebp

    tasks[task_count++] = task;

    LOG(DEBUG, "Done");
}

const char* loader_initrd_prefix = "/initrd/";

bool multitasking_add_task_from_initrd(char* name, const char* path, uint8_t ring, bool system, startup_data_struct_t* data)
{
    LOG(INFO, "Loading ELF file \"%s%s\"", loader_initrd_prefix, path);

    if (ring != 0 && ring != 3)
    {
        LOG(ERROR, "Invalid privilege level");
        return false;
    }

    initrd_file_t* file;
    if (!(file = initrd_find_file(path))) 
    {
        LOG(ERROR, "Coudln't load file");
        return false;
    }

    struct elf32_header* header = (struct elf32_header*)file->data;
    if (memcmp("\x7f""ELF", header->magic, 4) != 0) 
    {
        LOG(ERROR, "Invalid ELF signature");
        return false;
    }

    if (header->architecture != ELF_CLASS_32 || header->byte_order != ELF_DATA_LITTLE_ENDIAN || header->machine != ELF_INSTRUCTION_SET_x86)
    {
        LOG(ERROR, "Non x86 ELF file");
        return false;
    }

    if (header->type != ELF_TYPE_EXECUTABLE) 
    {
        LOG(ERROR, "Non executable ELF file");
        return false;
    }

    thread_t task = task_create_empty();
    int name_bytes = minint(strlen(name), THREAD_NAME_MAX - 1);
    memcpy(task.name, name, name_bytes);
    task.name[name_bytes] = 0;
    
    task.cr3 = vas_create_empty();

    task.esp = TASK_STACK_TOP_ADDRESS;

    task.ring = ring;

    task.system_task = system;

    LOG(DEBUG, "Entry point : 0x%x", header->entry);

    uint8_t code_segment = ring == 0 ? KERNEL_CODE_SEGMENT : USER_CODE_SEGMENT;

    {
        int i = 0;
        while (data->environ[i])
        {
            task_stack_push_string(&task, data->environ[i]);
            data->environ[i] = (char*)task.esp;
            i++;
        }
        i = 0;
        task_stack_push(&task, 0);
        while (data->environ[i])
        {
            task_stack_push(&task, (uint32_t)data->environ[i]);
            i++;
        }
        data->environ = (char**)task.esp;
    }

    task_stack_push_data(&task, (void*)data, sizeof(startup_data_struct_t));

    if (ring != 0)
    {
        uint32_t esp = task.esp;
        task_stack_push(&task, USER_DATA_SEGMENT);  // ss
        task_stack_push(&task, esp);                // esp
    }

    task_stack_push(&task, 0x200);          // eflags
    task_stack_push(&task, code_segment);   // cs
    task_stack_push(&task, header->entry);  // eip


    task_stack_push(&task, (uint32_t)iret_instruction);

    task_stack_push(&task, 0);      // ebx
    task_stack_push(&task, 0);      // esi
    task_stack_push(&task, 0);      // edi
    task_stack_push(&task, 0);      // ebp

    const int n_ph = header->phnum;

    for (elf32_half_t i = 0; i < n_ph; i++)
    {
        struct elf32_program_header* ph = (struct elf32_program_header*)&file->data[header->phoff + i * header->phentsize];
        if (ph->type == ELF_PROGRAM_TYPE_NULL) continue;
        if (ph->type != ELF_PROGRAM_TYPE_LOAD) 
        {
            LOG(ERROR, "Unsupported ELF program header type");
            abort();
        }

        LOG(DEBUG, "Program header %d : ", i);
        LOG(DEBUG, "├── Type : \"%s\"", ph->type >= sizeof(elf_program_header_type_string) / sizeof(char*) ? "UNKNOWN" : elf_program_header_type_string[ph->type]);
        LOG(DEBUG, "├── Virtual address : 0x%x", ph->p_vaddr);
        LOG(DEBUG, "├── File offset : 0x%x", ph->p_offset);
        LOG(DEBUG, "├── Memory size : %u bytes", ph->p_memsz);
        LOG(DEBUG, "└── File size : %u bytes", ph->p_filesz);

        virtual_address_t start_address = ph->p_vaddr & ~0xfff;
        virtual_address_t end_address = ph->p_vaddr + ph->p_memsz;
        uint32_t num_pages = (end_address - start_address + 0xfff) >> 12;

        // if (ph->p_memsz < ph->p_filesz)
        // {
        //     LOG(ERROR, "File size can never be larger than memory size");
        //     abort();
        // }

        // LOG(DEBUG, "0x%x : %u pages", start_address, num_pages);

        uint32_t address = start_address;
        for (uint32_t j = 0; j < num_pages; j++)
        {
            uint32_t pde = read_physical_address_4b(task.cr3 + ((address >> 22) * 4));
            physical_address_t pt_address = pde & 0xfffff000;
            if (!(pde & 1))
            {
                pt_address = pfa_allocate_physical_page();
                physical_init_page_table(pt_address);
                physical_add_page_table(task.cr3, (address >> 22), pt_address, PAGING_USER_LEVEL, true);
            }

            uint32_t pte = read_physical_address_4b(pt_address + (((address >> 12) & 0x3ff) * 4));
            physical_address_t page_address = pte & 0xfffff000;
            if (!(pte & 1))
            {
                page_address = pfa_allocate_physical_page();
                physical_set_page(pt_address, ((address >> 12) & 0x3ff), page_address, PAGING_USER_LEVEL, true);
            }

            for (uint16_t k = 0; k < 4096; k++)
            {
                int64_t data_offset = j * 4096 + k - (ph->p_vaddr & 0xfff);
                if (data_offset < 0 || data_offset >= ph->p_filesz)
                    write_physical_address_1b(page_address + k, 0);
                else
                    write_physical_address_1b(page_address + k, file->data[ph->p_offset + data_offset]);
            }

            address += 0x1000;
        }
    }

    tasks[task_count++] = task;

    LOG(INFO, "Loading successful");

    return true;
}

bool multitasking_add_task_from_vfs(char* name, const char* path, uint8_t ring, bool system, startup_data_struct_t* data)
{
    if (!name) return false;
    if (!data) abort();

    LOG(DEBUG, "Loading file \"%s\"", path);
    int i = 0;
    while (path[i] != 0 && loader_initrd_prefix[i] != 0 && path[i] == loader_initrd_prefix[i])
        i++;
    const size_t len = strlen(loader_initrd_prefix);
    if (i == len)
    {
        return multitasking_add_task_from_initrd(name, &path[len], ring, system, data);
    }
    LOG(ERROR, "Invalid path");
    return false;
}