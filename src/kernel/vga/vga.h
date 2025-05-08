#pragma once

void vga_write_port_3c0(uint8_t reg, uint8_t data)
{
    outb(0x3c0, reg);
    outb(0x3c0, data);
}

uint8_t vga_read_port_3c0(uint8_t reg)
{
    outb(0x3c0, reg);
    uint8_t data = inb(0x3c1);
    volatile uint8_t trash = inb(0x3da);
    return data;
}

void vga_write_misc_output_register(uint8_t data)
{
    outb(0x3c2, data);
}

uint8_t vga_read_misc_output_register()
{
    return inb(0x3cc);
}

void vga_write_dac_mask_register(uint8_t data)
{
    outb(0x3c6, data);
}

uint8_t vga_read_dac_mask_register()
{
    return inb(0x3c6);
}

void vga_write_port_3c4(uint8_t reg, uint8_t data)
{
    outb(0x3c4, reg);
    outb(0x3c5, data);
}

uint8_t vga_read_port_3c4(uint8_t reg)
{
    outb(0x3c4, reg);
    return inb(0x3c5);
}

void vga_write_port_3ce(uint8_t reg, uint8_t data)
{
    outb(0x3ce, reg);
    outb(0x3cf, data);
}

uint8_t vga_read_port_3ce(uint8_t reg)
{
    outb(0x3ce, reg);
    return inb(0x3cf);
}

void vga_write_port_3d4(uint8_t reg, uint8_t data)
{
    outb(0x3d4, reg);
    outb(0x3d5, data);
}

uint8_t vga_read_port_3d4(uint8_t reg)
{
    outb(0x3d4, reg);
    return inb(0x3d5);
}

void vga_init()
{
    vga_write_misc_output_register(vga_read_misc_output_register() | 1);    // Set registers to their default addresses
    volatile uint8_t trash = inb(0x3da);    // Reset port 3c0
}