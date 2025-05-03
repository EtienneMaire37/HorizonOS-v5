#pragma once

void flush_stdin()
{
    asm volatile("int 0xff" :: "a"(SYSCALL_FLUSH_INPUT_BUFFER));
}

bool set_kb_layout(int layout_idx)
{
    bool ret;
    asm volatile("int 0xff" : "=a"(ret) : "a"(SYSCALL_SET_KB_LAYOUT), "b"(layout_idx));
    return ret;
}