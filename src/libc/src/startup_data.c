#pragma once

startup_data_struct_t startup_data_init_from_command(char** cmd, char** envp, char* pwd)
{
    startup_data_struct_t data;

    data.cmd_line = cmd;
    data.environ = envp;
    data.pwd = pwd;

    return data;
}