#pragma once

struct ustar_header
{
    char    name[100];
    uint8_t mode[8];
    uint8_t owner_id[8];
    uint8_t group_id[8];
    char    size[12];
    uint8_t last_modification[12];
    uint8_t checksum[8];
    uint8_t type;
    uint8_t linked_file[100];
    uint8_t ustar[6];
    uint8_t version[2];
    uint8_t owner_name[32];
    uint8_t group_name[32];
    uint8_t device_major[8];
    uint8_t device_minor[8];
    uint8_t filename_prefix[155];
    uint8_t padding[12];
};

#define USTAR_BLOCK_SIZE 512

#define USTAR_TYPE_FILE '0' // Or 0
#define USTAR_TYPE_HARD_LINK '1'
#define USTAR_TYPE_SYMBOLIC_LINK '2'
#define USTAR_TYPE_CHARACTER_SPECIAL '3'
#define USTAR_TYPE_BLOCK_SPECIAL '4'
#define USTAR_TYPE_DIRECTORY '5'
#define USTAR_TYPE_FIFO '6'

#define USTAR_IS_VALID_HEADER(header) ((header).ustar[0] == 'u' && (header).ustar[1] == 's' && (header).ustar[2] == 't' && (header).ustar[3] == 'a' && (header).ustar[4] == 'r')

uint64_t ustar_get_number(char* str)
{
    uint64_t result = 0;
    uint64_t count = 1;

    for (uint8_t j = 11; j > 0; j--, count *= 8)
        result += ((str[j - 1] - '0') * count);

    return result;

}