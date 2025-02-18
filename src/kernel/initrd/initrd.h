#pragma once

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
        struct USTARHeader* header = (struct USTARHeader*)(initrd_start + initrd_offset);

        if (header->name[0] == '\0')
        {
            // LOG(INFO, "End of USTAR archive");
            break;
        }

        if (!USTAR_IS_VALID_HEADER(*header))
        {
            LOG(WARNING, "Invalid USTAR header at offset %u", initrd_offset);
            break;
        }

        uint32_t file_size = ustar_get_number(header->size);    // 32bit here but theoretically it should be 64bit

        LOG(INFO, "    File : %s ; Size : %u bytes", header->name, file_size);

        initrd_offset += (file_size + USTAR_BLOCK_SIZE - 1) / USTAR_BLOCK_SIZE * USTAR_BLOCK_SIZE + USTAR_BLOCK_SIZE;
    }

    LOG(INFO, "Done parsing initrd");
}