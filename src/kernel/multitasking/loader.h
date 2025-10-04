#pragma once

void multitasking_add_task_from_function(char* name, void (*func)());
bool multitasking_add_task_from_initrd(char* name, const char* path, uint8_t ring, bool system, startup_data_struct_t* data);
bool multitasking_add_task_from_vfs(char* name, const char* path, uint8_t ring, bool system, startup_data_struct_t* data);