#pragma once

// ! VGA Registers are not guaranteed to work under UEFI

#define VGA_REG_3C4_RESET                 0x00
#define VGA_REG_3C4_CLOCKING_MODE         0x01
#define VGA_REG_3C4_MAP_MASK              0x02
#define VGA_REG_3C4_CHARACTER_MAP_SELECT  0x03
#define VGA_REG_3C4_MEMORY_MODE           0x04

#define VGA_REG_3CE_SET_RESET             0x00
#define VGA_REG_3CE_ENABLE_SET_RESET      0x01
#define VGA_REG_3CE_COLOR_COMPARE         0x02
#define VGA_REG_3CE_DATA_ROTATE           0x03
#define VGA_REG_3CE_READ_MAP_SELECT       0x04
#define VGA_REG_3CE_GRAPHICS_MODE         0x05
#define VGA_REG_3CE_MISCELLANEOUS         0x06
#define VGA_REG_3CE_COLOR_DONT_CARE       0x07
#define VGA_REG_3CE_BIT_MASK              0x08

#define VGA_REG_3D4_HORIZONTAL_TOTAL          0x00
#define VGA_REG_3D4_HORIZONTAL_DISPLAY_END    0x01
#define VGA_REG_3D4_START_HORIZONTAL_BLANKING 0x02
#define VGA_REG_3D4_END_HORIZONTAL_BLANKING   0x03
#define VGA_REG_3D4_START_HORIZONTAL_RETRACE  0x04
#define VGA_REG_3D4_END_HORIZONTAL_RETRACE    0x05
#define VGA_REG_3D4_VERTICAL_TOTAL            0x06
#define VGA_REG_3D4_OVERFLOW                  0x07
#define VGA_REG_3D4_PRESET_ROW_SCAN           0x08
#define VGA_REG_3D4_MAXIMUM_SCAN_LINE         0x09
#define VGA_REG_3D4_CURSOR_START              0x0A
#define VGA_REG_3D4_CURSOR_END                0x0B
#define VGA_REG_3D4_START_ADDRESS_HIGH        0x0C
#define VGA_REG_3D4_START_ADDRESS_LOW         0x0D
#define VGA_REG_3D4_CURSOR_LOCATION_HIGH      0x0E
#define VGA_REG_3D4_CURSOR_LOCATION_LOW       0x0F
#define VGA_REG_3D4_VERTICAL_RETRACE_START    0x10
#define VGA_REG_3D4_VERTICAL_RETRACE_END      0x11
#define VGA_REG_3D4_VERTICAL_DISPLAY_END      0x12
#define VGA_REG_3D4_LOGICAL_WIDTH             0x13
#define VGA_REG_3D4_UNDERLINE_LOCATION        0x14
#define VGA_REG_3D4_VERTICAL_BLANK_START      0x15
#define VGA_REG_3D4_VERTICAL_BLANK_END        0x16
#define VGA_REG_3D4_MODE_CONTROL              0x17
#define VGA_REG_3D4_LINE_COMPARE              0x18

void vga_write_port_3c0(uint8_t reg, uint8_t data)
{
    abort();
    outb(0x3c0, reg);
    outb(0x3c0, data);
}

uint8_t vga_read_port_3c0(uint8_t reg)
{
    abort();
    outb(0x3c0, reg);
    uint8_t data = inb(0x3c1);
    inb(0x3da);
    return data;
}

void vga_write_misc_output_register(uint8_t data)
{
    abort();
    outb(0x3c2, data);
}

uint8_t vga_read_misc_output_register()
{
    abort();
    return inb(0x3cc);
}

void vga_write_dac_mask_register(uint8_t data)
{
    abort();
    outb(0x3c6, data);
}

uint8_t vga_read_dac_mask_register()
{
    abort();
    return inb(0x3c6);
}

void vga_write_port_3c4(uint8_t reg, uint8_t data)
{
    abort();
    outb(0x3c4, reg);
    outb(0x3c5, data);
}

uint8_t vga_read_port_3c4(uint8_t reg)
{
    abort();
    outb(0x3c4, reg);
    return inb(0x3c5);
}

void vga_write_port_3ce(uint8_t reg, uint8_t data)
{
    abort();
    outb(0x3ce, reg);
    outb(0x3cf, data);
}

uint8_t vga_read_port_3ce(uint8_t reg)
{
    abort();
    outb(0x3ce, reg);
    return inb(0x3cf);
}

void vga_write_port_3d4(uint8_t reg, uint8_t data)
{
    abort();
    outb(0x3d4, reg);
    outb(0x3d5, data);
}

uint8_t vga_read_port_3d4(uint8_t reg)
{
    abort();
    outb(0x3d4, reg);
    return inb(0x3d5);
}

void vga_init()
{
    abort();
    vga_write_misc_output_register(vga_read_misc_output_register() | 1);    // Set registers to their default addresses
    inb(0x3da);    // Reset port 3c0
}