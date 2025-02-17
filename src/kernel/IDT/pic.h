#pragma once

#define PIC1		    0x20
#define PIC2		    0xA0
#define PIC1_CMD	    (PIC1 + 0)
#define PIC1_DATA	    (PIC1 + 1)
#define PIC2_CMD    	(PIC2 + 0)
#define PIC2_DATA	    (PIC2 + 1)

#define PIC_EOI		    0x20

#define PIC_READ_IRR                0x0a
#define PIC_READ_ISR                0x0b
 
void pic_send_eoi(uint8_t irq);
void pic_disable();
void pic_remap(uint8_t master_offset, uint8_t slave_offset);
void pic_set_mask(uint16_t mask);
uint16_t pic_get_register(int32_t OCW3);
uint16_t pic_get_irr();
uint16_t pic_get_isr();