#pragma once

typedef struct __attribute__((packed)) startup_data_struct
{
    char cmd_line[4096];
    char pwd[PATH_MAX];
    char** environ;
} startup_data_struct_t;

startup_data_struct_t startup_data_init_from_command(const char* cmd, char** envp, const char* pwd);
startup_data_struct_t startup_data_init_from_argv(const char** argv, char** envp, const char* pwd);