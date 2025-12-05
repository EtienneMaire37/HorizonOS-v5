#pragma once

static uint8_t wstatus_return_value;

#define WEXITSTATUS(wstatus)    (wstatus_return_value = (uint8_t)wstatus, *(int8_t*)&wstatus_return_value)
#define WTERMSIG(wstatus)       (wstatus_return_value = (uint8_t)wstatus, *(int8_t*)&wstatus_return_value)

#define WEXITBIT                0x00010000
#define WSIGNALBIT              0x00020000

#define WIFEXITED(wstatus)      ((unsigned int)wstatus & WEXITBIT)
#define WIFSIGNALED(wstatus)    ((unsigned int)wstatus & WSIGNALBIT)

pid_t waitpid(pid_t pid, int* wstatus, int options);