#pragma once

#include <stddef.h>
#include <errno.h>
#include "../multicore/spinlock.h"
#include "open_file.h"

typedef struct
{
    open_file_descriptor_entry_t* data;
    size_t allocated, size;
    spinlock_noint_t lock;
} open_file_descriptor_vector_t;

#define OPEN_FILE_DESCRIPTOR_ENTRIES_VECTOR_INIT    ((open_file_descriptor_vector_t){ .allocated = 0, .size = 0, .data = NULL })

int ofdv_open(open_file_descriptor_entry_t* stack, open_file_descriptor_entry_t entry);
int ofdv_close(open_file_descriptor_vector_t* stack, int fd);

