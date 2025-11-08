#pragma once

#include "loader.h"
#include "vas.h"
#include "../files/elf.h"
#include "../../libc/src/startup_data.h"

void multitasking_add_task_from_function(const char* name, void (*func)())
{
    LOG(DEBUG, "Adding task \"%s\" from function", name);

    thread_t task = task_create_empty();
    task_set_name(&task, name);
    task.cr3 = task_create_empty_vas(PG_SUPERVISOR);

    task_setup_stack(&task, (uint64_t)func, KERNEL_CODE_SEGMENT, KERNEL_DATA_SEGMENT);

    tasks[task_count++] = task;

    LOG(DEBUG, "Done");
}

bool multitasking_add_task_from_initrd(const char* name, const char* path, uint8_t ring, bool system, startup_data_struct_t* data)
{
    LOG(INFO, "Loading ELF file \"/initrd/%s\"", path);

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

    elf64_header_t* header = (elf64_header_t*)file->data;
    if (memcmp("\x7f""ELF", header->magic, 4) != 0) 
    {
        LOG(ERROR, "Invalid ELF signature");
        return false;
    }

    if (header->architecture != ELF_CLASS_64 || header->byte_order != ELF_DATA_LITTLE_ENDIAN || header->machine != ELF_INSTRUCTION_SET_x86_64)
    {
        LOG(ERROR, "Non x86_64 ELF file");
        return false;
    }

    if (header->type != ELF_TYPE_EXECUTABLE) 
    {
        LOG(ERROR, "Non executable ELF file");
        return false;
    }

    thread_t task = task_create_empty();
    task_set_name(&task, name);
    task.cr3 = task_create_empty_vas(ring == 0 ? PG_SUPERVISOR : PG_USER);

    task_setup_stack(&task, header->entry, 
        ring == 0 ? KERNEL_CODE_SEGMENT : USER_CODE_SEGMENT, 
        ring == 0 ? KERNEL_DATA_SEGMENT : USER_DATA_SEGMENT);

    task.ring = ring;

    task.system_task = system;

    LOG(DEBUG, "Entry point : 0x%llx", header->entry);

    // !!! TODO: Pass the startup data struct to the program

    const int n_ph = header->phnum;

    for (elf64_half_t i = 0; i < n_ph; i++)
    {
        elf64_program_header_t* ph = (elf64_program_header_t*)&file->data[header->phoff + i * header->phentsize];
        if (ph->type == ELF_PROGRAM_TYPE_NULL) continue;
        if (ph->type != ELF_PROGRAM_TYPE_LOAD) 
        {
            LOG(ERROR, "Unsupported ELF program header type");
            abort();
        }

        LOG(DEBUG, "Program header %u : ", i);
        LOG(DEBUG, "├── Type : \"%s\"", ph->type >= sizeof(elf_program_header_type_string) / sizeof(char*) ? "UNKNOWN" : elf_program_header_type_string[ph->type]);
        LOG(DEBUG, "├── Virtual address : 0x%llx", ph->p_vaddr);
        LOG(DEBUG, "├── File offset : 0x%llx", ph->p_offset);
        LOG(DEBUG, "├── Memory size : %u bytes", ph->p_memsz);
        LOG(DEBUG, "└── File size : %u bytes", ph->p_filesz);

        virtual_address_t start_address = ph->p_vaddr & ~0xfff;
        virtual_address_t end_address = ph->p_vaddr + ph->p_memsz;
        uint64_t num_pages = (end_address - start_address + 0xfff) >> 12;

        // LOG(DEBUG, "0x%llx : %llu pages", start_address, num_pages);

        // TODO: allocate_range, find physical addresses and copy_page them
    }

    tasks[task_count++] = task;

    LOG(DEBUG, "Done");

    return true;
}

bool multitasking_add_task_from_vfs(const char* name, const char* path, uint8_t ring, bool system, startup_data_struct_t* data)
{
    if (!name) return false;
    if (!data) abort();

    LOG(DEBUG, "Loading file \"%s\"", path);
    switch (get_drive_type(path))
    {
    case DT_INITRD:
        return multitasking_add_task_from_initrd(name, &path[strlen("/initrd/")], ring, system, data);
    default:
        LOG(ERROR, "Invalid path");
        return false;
    }
}