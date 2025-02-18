#pragma once

struct USTARHeader
{
    char name[100];
    char mode[8];
    char owner_id[8];
    char group_id[8];
    char size[12];
    char last_modification[12];
    char checksum[8];
    char type;
    char linked_file[100];
    char ustar[6];
    char version[2];
    char owner_name[32];
    char group_name[32];
    char device_major[8];
    char device_minor[8];
    char filename_prefix[155];
    char padding[12];
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