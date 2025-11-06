#pragma once

void idle_main()
{
    while(true) 
    {
        printf("Hello from idle task!!!!\n");
        // hlt();
    }
    __builtin_unreachable();
}