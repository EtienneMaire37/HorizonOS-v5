#pragma once

#define CMOS_REGISTER_SELECT    0x70
#define CMOS_REGISTER_DATA      0x71

#define cmos_select_register(reg)   outb(CMOS_REGISTER_SELECT, reg | 0x80)    // Disable NMI
#define cmos_read_register()        inb(CMOS_REGISTER_DATA)
#define cmos_write_register(data)   outb(CMOS_REGISTER_DATA, data)

#define CMOS_REGISTER_SECONDS    0x00
#define CMOS_REGISTER_MINUTES    0x02
#define CMOS_REGISTER_HOURS      0x04
#define CMOS_REGISTER_DAY        0x07
#define CMOS_REGISTER_MONTH      0x08
#define CMOS_REGISTER_YEAR       0x09
#define CMOS_REGISTER_STATUS_A   0x0a
#define CMOS_REGISTER_STATUS_B   0x0b

#define CMOS_CENTURY_REGISTER    0x32

#define cmos_wait_while_updating()  { do { cmos_select_register(CMOS_REGISTER_STATUS_A); } while(!(cmos_read_register() & 0x80)); do { cmos_select_register(CMOS_REGISTER_STATUS_A); } while(cmos_read_register() & 0x80); }