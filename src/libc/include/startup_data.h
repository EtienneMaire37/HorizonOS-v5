#pragma once

typedef struct __attribute__((packed)) startup_data_struct
{
    char cmd_line[4096];
} startup_data_struct_t;

startup_data_struct_t startup_data_init(const char* cmd)
{
    startup_data_struct_t data;
    memcpy(data.cmd_line, cmd, minint(strlen(cmd), 4095));
    data.cmd_line[4095] = 0;
    return data;
}