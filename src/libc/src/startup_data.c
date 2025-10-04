#pragma once

startup_data_struct_t startup_data_init_from_command(const char* cmd, char** envp, const char* pwd)
{
    startup_data_struct_t data;

    if (cmd)
    {
        memcpy(data.cmd_line, cmd, 4095);
        data.cmd_line[4095] = 0;
    }
    else
        memset(data.cmd_line, 0, 4096);

    if (pwd)
    {
        memcpy(data.pwd, pwd, PATH_MAX - 1);
        data.pwd[PATH_MAX - 1] = 0;
    }
    else
        memset(data.pwd, 0, PATH_MAX);

    if (envp)
        data.environ = envp;
    else
        data.environ = (char*[]){NULL};
    return data;
}

startup_data_struct_t startup_data_init_from_argv(const char** argv, char** envp, const char* pwd)
{
    char cmd_line[4096];
    char* cmd = cmd_line;
    if (!argv) cmd = NULL;
    else
    {
        int i = 0, j = 0, k = 0;
        while (argv[i] && j < 4095)
        {
            if (j >= 4095) continue;
            cmd_line[j++] = '\"';
            k = 0;
            while (j < 4095 && argv[i][k] != 0)
            {
                cmd_line[j++] = argv[i][k];
                k++;
            }
            if (j >= 4095) continue;
            cmd_line[j++] = '\"';
            i++;
        }
    }
    return startup_data_init_from_command(cmd, envp, pwd);
}