#pragma once

#define SYSCALL_EXIT        0
#define SYSCALL_TIME        1
#define SYSCALL_READ        2
#define SYSCALL_WRITE       3
#define SYSCALL_GETPID      4
#define SYSCALL_FORK        5
#define SYSCALL_BRK_FREE    6
#define SYSCALL_BRK_ALLOC   7
#define SYSCALL_EXECVE      8
#define SYSCALL_WAITPID     9
#define SYSCALL_ACCESS      10
#define SYSCALL_STAT        11
#define SYSCALL_READDIR     12

#define SYSCALL_FLUSH_INPUT_BUFFER  0x40
#define SYSCALL_SET_KB_LAYOUT       0x8000