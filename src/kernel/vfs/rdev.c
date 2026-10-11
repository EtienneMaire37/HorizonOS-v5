#include "rdev.h"

dev_t vfs_generate_rdev()
{
    static dev_t num = 1;
    return num++;
}

