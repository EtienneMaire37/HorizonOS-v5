#pragma once
 
void pic_send_eoi(uint8_t irq)
{
	if(irq > 0b111) // Slave PIC
    {
        outb(PIC2_CMD, PIC_EOI);
        io_wait();
    }
 
    // Master PIC
	outb(PIC1_CMD, PIC_EOI);
    io_wait();
}

void pic_disable() 
{
    pic_set_mask(0xffff);
}

void pic_remap(uint8_t master_offset, uint8_t slave_offset)
{
    // Init sequence
    outb(PIC1_CMD, 0x11);
    io_wait();
    outb(PIC2_CMD, 0x11);
    io_wait();
    outb(PIC1_DATA, master_offset);
    io_wait();
    outb(PIC2_DATA, slave_offset);
    io_wait();
    outb(PIC1_DATA, 0x04);
    io_wait();
    outb(PIC2_DATA, 0x02);
    io_wait();
    outb(PIC1_DATA, 0x01);
    io_wait();
    outb(PIC2_DATA, 0x01);
    io_wait();

    // No masking 
    pic_set_mask(0);
}

void pic_set_mask(uint16_t mask) 
{
    outb(PIC1_DATA, mask & 0xff);      
	io_wait();
	outb(PIC2_DATA, mask >> 8);      
	io_wait();
}
 
uint16_t pic_get_register(int32_t OCW3)
{
    outb(PIC1_CMD, OCW3);
    outb(PIC2_CMD, OCW3);
    return (inb(PIC2_CMD) << 8) | inb(PIC1_CMD);
}
 
uint16_t pic_get_irr()
{
    return pic_get_register(PIC_READ_IRR);
}
 
uint16_t pic_get_isr()
{
    return pic_get_register(PIC_READ_ISR);
}