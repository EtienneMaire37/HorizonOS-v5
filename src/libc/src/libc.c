#define BUILDING_C_LIB

#include <stdbool.h>
// #include <stdint.h>
#include <stdatomic.h>

#define min(a, b)   ((a) > (b) ? (b) : (a))
#define max(a, b)   ((a) < (b) ? (b) : (a))

#include "../../kernel/multicore/spinlock.h"

#include "../include/errno.h"
#include "../include/inttypes.h"
#include "../include/unistd.h"
#include "../include/stdio.h"
#include "../include/stdlib.h"
#include "../include/time.h"
#include "../include/stdarg.h"
#include "../include/string.h"
#include "../include/sys/types.h"
#include "../include/horizonos.h"

#include "kernel_glue.h"

FILE* FILE_create()
{
    FILE* f = (FILE*)malloc(sizeof(FILE));
    if (!f) return NULL;
    f->buffer = (uint8_t*)malloc(BUFSIZ);
    if (!f->buffer)
    {
        free(f);
        return NULL;
    }
    f->fd = -1;
    f->buffer_size = BUFSIZ;
    f->buffer_index = 0;
    f->buffer_mode = 0;
    f->flags = FILE_FLAGS_BF_ALLOC;
    f->current_flags = 0;
    f->buffer_end_index = 0;
    return f;
}

#include "unistd.c"
#include "stdio.c"
#include "stdlib.c"
#include "time.c"
#include "string.c"
#include "arithmetic.c"

#include "liballoc.h"
#include "liballoc_hooks.c"
#include "liballoc.c"

#include "kernel_glue.c"
#include "horizonos.c"

#include "crt0.c"