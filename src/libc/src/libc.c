#include <stdbool.h>
// #include <stdint.h>
#include <stdatomic.h>

#include "../../kernel/multicore/spinlock.h"

#include "../include/inttypes.h"
#include "../include/unistd.h"
#include "../include/stdio.h"
#include "../include/stdlib.h"
#include "../include/time.h"
#include "../include/stdarg.h"
#include "../include/string.h"
#include "../include/errno.h"
#include "../include/sys/types.h"
#include "../include/horizonos.h"

#include "kernel_glue.h"

#include "unistd.c"
#include "stdio.c"
#include "stdlib.c"
#include "time.c"
#include "string.c"
#include "arithmetic.c"
#include "crt0.c"

#include "liballoc_hooks.c"
#include "liballoc.c"

#include "kernel_glue.c"
#include "horizonos.c"