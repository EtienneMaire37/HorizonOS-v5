#pragma once

typedef uint8_t tar_file_type;

struct initrd_file
{
    char* name;
    uint32_t size;
    uint8_t* data;
    tar_file_type type;

    // 12 bytes
};

#define MAX_INITRD_FILES 32

struct initrd_file initrd_files[MAX_INITRD_FILES];
uint8_t initrd_files_count = 0;

void initrd_parse()
{
    LOG(INFO, "Parsing initrd");

    uint32_t initrd_start = initrd_module->mod_start;
    uint32_t initrd_end = initrd_module->mod_end;

    uint32_t initrd_size = initrd_end - initrd_start;

    LOG(INFO, "Initrd size : %u bytes", initrd_size);

    uint32_t initrd_offset = 0;

    while (initrd_offset < initrd_size)
    {
        struct ustar_header* header = (struct ustar_header*)(initrd_start + initrd_offset);

        if (header->name[0] == '\0')
            break;

        if (!USTAR_IS_VALID_HEADER(*header))
        {
            LOG(WARNING, "Invalid USTAR header at offset %u", initrd_offset);
            break;
        }

        uint64_t file_size = ustar_get_number(header->size);

        initrd_files[initrd_files_count].name = &header->name[0];
        initrd_files[initrd_files_count].size = file_size;
        initrd_files[initrd_files_count].data = (uint8_t*)(header + 1); // 512 bytes after the header
        if (header->type == 0) header->type = '0';
        initrd_files[initrd_files_count].type = header->type;
        initrd_files_count++;

        initrd_offset += (file_size + USTAR_BLOCK_SIZE - 1) / USTAR_BLOCK_SIZE * USTAR_BLOCK_SIZE + USTAR_BLOCK_SIZE;
    }

    for (uint8_t i = 0; i < initrd_files_count; i++)
    {
        LOG(INFO, "%s── File : %s ; Size : %u bytes", initrd_files_count - i > 1 ? "├" : "└", initrd_files[i].name, initrd_files[i].size);
    }

    LOG(INFO, "Done parsing initrd (%u files)", initrd_files_count);
}

struct initrd_file* initrd_find_file(char* name)
{
    LOG(INFO, "Opening file \"%s\" from initrd", name);

    for (uint8_t i = 0; i < initrd_files_count; i++)
    {
        if (strcmp(initrd_files[i].name, name) == 0)
            return &initrd_files[i];
    }

    LOG(INFO, "Error opening file \"%s\"", name);

    return NULL;
}