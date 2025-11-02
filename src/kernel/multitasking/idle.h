#pragma once

void idle_main()
{
    while(true) hlt();
    __builtin_unreachable();
}