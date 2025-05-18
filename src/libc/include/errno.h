#pragma once

#include "errno_defs.h"

#ifdef BUILDING_C_LIB
int errno;
#else
extern int errno;
#endif