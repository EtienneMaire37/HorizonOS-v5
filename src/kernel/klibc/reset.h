#pragma once

void reset()
{
    // ! Triple fault method
    LOG(WARNING, "Rebooting...");

    _idtr.size = 0;
    load_idt();

    enable_interrupts();

    while (true);
}