#pragma once

typedef struct __attribute__((packed)) startup_data_struct
{
    char cmd_line[4096];
    char pwd[PATH_MAX];
} startup_data_struct_t;

startup_data_struct_t startup_data_init_from_command(const char* cmd, const char* pwd)
{
    startup_data_struct_t data;
    memcpy(data.cmd_line, cmd, minint(strlen(cmd), 4095));
    data.cmd_line[4095] = 0;
    memcpy(data.pwd, pwd, minint(strlen(pwd), PATH_MAX - 1));
    data.cmd_line[PATH_MAX - 1] = 0;
    return data;
}

startup_data_struct_t startup_data_init_from_argv(const char** argv, const char* pwd)
{
    startup_data_struct_t data;
    if (!argv) data.cmd_line[0] = 0;
    else
    {
        int i = 0, j = 0, k = 0;
        while (argv[i] && j < 4095)
        {
            if (j >= 4095) continue;
            data.cmd_line[j++] = '\"';
            k = 0;
            while (j < 4095 && argv[i][k] != 0)
            {
                data.cmd_line[j++] = argv[i][k];
                k++;
            }
            if (j >= 4095) continue;
            data.cmd_line[j++] = '\"';
            i++;
        }
    }
    data.cmd_line[4095] = 0;
    memcpy(data.pwd, pwd, minint(strlen(pwd), PATH_MAX - 1));
    data.cmd_line[PATH_MAX - 1] = 0;
    return data;
}