#pragma once

#include "../files/ustar.h"
#include "../../libc/include/unistd.h"
#include "../debug/out.h"
#include "../../libc/include/stdio.h"
#include "../../libc/include/string.h"
#include "../../libc/include/sys/stat.h"

typedef uint8_t tar_file_type;

typedef struct initrd_file
{
    char* name;
    uint64_t size;
    uint8_t* data;
    tar_file_type type;
    char* link;
    mode_t mode;
} initrd_file_t;

#define MAX_INITRD_FILES 32

initrd_file_t initrd_files[MAX_INITRD_FILES];
uint8_t initrd_files_count = 0;

void initrd_parse(uintptr_t initrd_start, uintptr_t initrd_end)
{
    LOG(INFO, "Parsing initrd");

    intptr_t initrd_size = initrd_end - initrd_start;

    LOG(INFO, "Initrd size : %u bytes", initrd_size);

    intptr_t initrd_offset = 0;

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

        uint64_t file_size = ustar_get_number(header->size, 12);

        // if (header->type != USTAR_TYPE_FILE_1 && header->type != USTAR_TYPE_FILE_2 && header->type != USTAR_TYPE_DIRECTORY)
        //     continue;

        initrd_files[initrd_files_count].name = &header->name[0];
        size_t len = strlen(initrd_files[initrd_files_count].name);
        if (len >= 1 && initrd_files[initrd_files_count].name[len - 1] == '/')
            initrd_files[initrd_files_count].name[len - 1] = 0;
        if (strcmp(initrd_files[initrd_files_count].name, ".") != 0)
        {
            initrd_files[initrd_files_count].size = file_size;
            initrd_files[initrd_files_count].data = (uint8_t*)(header + 1); // 512 bytes after the header
            if (header->type == 0) header->type = '0';

            initrd_files[initrd_files_count].type = header->type;
            initrd_files[initrd_files_count].link = (char*)&header->linked_file[0];

            uint64_t mode = ustar_get_number((char*)header->mode, 8);
            initrd_files[initrd_files_count].mode = (header->type == USTAR_TYPE_DIRECTORY ? S_IFDIR : S_IFREG) | 
            ((mode & TUREAD) ? S_IRUSR : 0) | ((mode & TUEXEC) ? S_IXUSR : 0) | // * | ((mode & TUWRITE) ? S_IWUSR : 0)
            ((mode & TGREAD) ? S_IRGRP : 0) | ((mode & TGEXEC) ? S_IXGRP : 0) | // * | ((mode & TGWRITE) ? S_IWGRP : 0)
            ((mode & TOREAD) ? S_IROTH : 0) | ((mode & TOEXEC) ? S_IXOTH : 0);  // * | ((mode & TOWRITE) ? S_IWOTH : 0)

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
        {
            LOG(DEBUG, "Found at index %u", i);
            return &initrd_files[i];
        }
    }

    LOG(INFO, "Error opening file \"%s\"", name);

    return NULL;
}

initrd_file_t* initrd_find_file_entry(const char* name)
{
    LOG(INFO, "Opening file entry \"%s\" from initrd", name);

    for (uint8_t i = 0; i < initrd_files_count; i++)
    {
        if (strcmp(initrd_files[i].name, name) == 0)
        {
            LOG(DEBUG, "Found at index %u", i);
            return &initrd_files[i];
        }
    }

    LOG(INFO, "Error opening file \"%s\"", name);

    return NULL;
}