#pragma once

#define PS2_DATA_PORT         0x60
#define PS2_STATUS_REGISTER   0x64
#define PS2_COMMAND_REGISTER  0x64

#define ps2_ready() ((inb(PS2_STATUS_REGISTER) & 0b10) == 0)
#define ps2_wait() while(!ps2_ready())