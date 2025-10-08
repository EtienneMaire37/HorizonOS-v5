#define BUILDING_C_LIB

char** environ;

#include <stdbool.h>
// #include <stdint.h>
#include <stdatomic.h>

#include "../include/inttypes.h"

int64_t minint(int64_t a, int64_t b)
{
    return a < b ? a : b;
}
int64_t maxint(int64_t a, int64_t b)
{
    return a > b ? a : b;
}

#include "../../kernel/multicore/spinlock.h"

#include "../include/limits.h"

char cwd[PATH_MAX] = {0};

#include "../include/errno.h"
#include "../include/unistd.h"
#include "../include/stdio.h"
#include "../include/sys/wait.h"
#include "../include/stdlib.h"
#include "../include/time.h"
#include "../include/stdarg.h"
#include "../include/string.h"
#include "../include/sys/types.h"
#include "../include/horizonos.h"
#include "../include/sys/stat.h"

#include "startup_data.h"

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

#include "liballoc.h"
#include "liballoc_hooks.c"
#include "liballoc.c"

#include "kernel_glue.c"
#include "horizonos.c"

#include "crt0.c"