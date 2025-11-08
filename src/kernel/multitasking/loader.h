#pragma once

#include "../../libc/src/startup_data.h"

void multitasking_add_task_from_function(const char* name, void (*func)());
bool multitasking_add_task_from_initrd(const char* name, const char* path, uint8_t ring, bool system, startup_data_struct_t* data);
bool multitasking_add_task_from_vfs(const char* name, const char* path, uint8_t ring, bool system, startup_data_struct_t* data);