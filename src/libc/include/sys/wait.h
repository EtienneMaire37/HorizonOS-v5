#pragma once

static uint8_t wstatus_return_value;

#define WEXITSTATUS(wstatus)    (wstatus_return_value = (uint8_t)wstatus, *(int8_t*)&wstatus_return_value)
#define WIFEXITED(wstatus)      (!((unsigned int)wstatus & 0x80000000))

pid_t waitpid(pid_t pid, int* wstatus, int options);