#pragma once

#include "errno_defs.h"

#ifdef BUILDING_C_LIB
// __thread
int errno;
#else
extern int errno;
#endif