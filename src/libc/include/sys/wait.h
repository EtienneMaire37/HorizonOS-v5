#pragma once

#define WEXITSTATUS(wstatus) wstatus

pid_t waitpid(pid_t pid, int* wstatus, int options);