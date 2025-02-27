#pragma once

void putchar(char c)
{
    asm("int 0xff" : : "a"(c & 0x7f));
}