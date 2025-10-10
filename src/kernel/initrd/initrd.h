#pragma once

typedef uint8_t tar_file_type;

typedef struct initrd_file
{
    char* name;
    uint32_t size;
    uint8_t* data;
    tar_file_type type;
    uint8_t* link;
} initrd_file_t;

#define MAX_INITRD_FILES 32

initrd_file_t initrd_files[MAX_INITRD_FILES];
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

        // if (header->type != USTAR_TYPE_FILE_1 && header->type != USTAR_TYPE_FILE_2 && header->type != USTAR_TYPE_DIRECTORY)
        //     continue;

        initrd_files[initrd_files_count].name = &header->name[0];
        int str_len = strlen(initrd_files[initrd_files_count].name);
        for (int i = 0; i < str_len; i++)
        {
            if (initrd_files[initrd_files_count].name[i] == '/' && !initrd_files[initrd_files_count].name[i + 1])
                initrd_files[initrd_files_count].name[i] = 0;
        }
        if (strcmp(initrd_files[initrd_files_count].name, ".") != 0)
        {
            initrd_files[initrd_files_count].size = file_size;
            initrd_files[initrd_files_count].data = (uint8_t*)(header + 1); // 512 bytes after the header
            if (header->type == 0) header->type = '0';
            initrd_files[initrd_files_count].type = header->type;
            initrd_files[initrd_files_count].link = &header->linked_file[0];
            initrd_files_count++;
        }
        initrd_offset += (file_size + USTAR_BLOCK_SIZE - 1) / USTAR_BLOCK_SIZE * USTAR_BLOCK_SIZE + USTAR_BLOCK_SIZE;
    }

    for (uint8_t i = 0; i < initrd_files_count; i++)
    {
        // if (initrd_files[i].size != 0 && 
        //     initrd_files[i].name != NULL)
        {
            char* tree_inter = initrd_files_count - i > 1 ? "├" : "└";
            tar_file_type this_type = initrd_files[i].type;
            switch (this_type)
            {
            case USTAR_TYPE_FILE_1:
            case USTAR_TYPE_FILE_2:
                LOG(INFO, "%s── File : \"%s\" ; Size : %u bytes", tree_inter, initrd_files[i].name, initrd_files[i].size);
                break;
            case USTAR_TYPE_HARD_LINK:
                LOG(INFO, "%s── Hard link : \"%s\" pointing to \"%s\"", tree_inter, initrd_files[i].name, initrd_files[i].link);
                break;
            case USTAR_TYPE_SYMBOLIC_LINK:
                LOG(INFO, "%s── Symbolic link : \"%s\" pointing to \"%s\"", tree_inter, initrd_files[i].name, initrd_files[i].link);
                break;
            case USTAR_TYPE_CHARACTER_DEVICE:
                LOG(INFO, "%s── Character device : \"%s\"", tree_inter, initrd_files[i].name);
                break;
            case USTAR_TYPE_BLOCK_DEVICE:
                LOG(INFO, "%s── Block device : \"%s\"", tree_inter, initrd_files[i].name);
                break;
            case USTAR_TYPE_DIRECTORY:
                LOG(INFO, "%s── Directory : \"%s\"", tree_inter, initrd_files[i].name);
                break;
            case USTAR_TYPE_NAMED_PIPE:
                LOG(INFO, "%s── FIFO : \"%s\"", tree_inter, initrd_files[i].name);
                break;
            default:
                ;
            }
        }
    }

    LOG(INFO, "Done parsing initrd (%u files)", initrd_files_count);
}

initrd_file_t* initrd_find_file(const char* name)
{
    LOG(INFO, "Opening file \"%s\" from initrd", name);

    for (uint8_t i = 0; i < initrd_files_count; i++)
    {
        if (strcmp(initrd_files[i].name, name) == 0 && initrd_files[i].type == USTAR_TYPE_FILE_1)
            return &initrd_files[i];
    }

    LOG(INFO, "Error opening file \"%s\"", name);

    return NULL;
}

initrd_file_t* initrd_find_file_entry(const char* name)
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