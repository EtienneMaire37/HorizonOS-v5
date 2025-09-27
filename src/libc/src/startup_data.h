#pragma once

typedef struct __attribute__((packed)) startup_data_struct
{
    char cmd_line[4096];
    char pwd[PATH_MAX];
} startup_data_struct_t;

startup_data_struct_t startup_data_init(const char* cmd, const char* pwd)
{
    startup_data_struct_t data;
    memcpy(data.cmd_line, cmd, minint(strlen(cmd), 4095));
    data.cmd_line[4095] = 0;
    memcpy(data.pwd, pwd, minint(strlen(pwd), PATH_MAX - 1));
    data.cmd_line[PATH_MAX - 1] = 0;
    return data;
}