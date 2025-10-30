#include <stdint.h>

#include <bootboot.h>

int _gdtr, _idtr;

void interrupt_handler()
{
    ;
}

void halt()
{
    while (1) 
    {
        asm volatile("cli");
        asm volatile("hlt");
    }
    __builtin_unreachable();
}

extern BOOTBOOT bootboot;
extern unsigned char environment[4096];
extern uint8_t fb;

void puts(char* s);

void _start()
{
    if (!bootboot.fb_scanline) 
    {
        halt();
    }

    puts("Hello from a simple BOOTBOOT kernel");
    
    halt();
}

typedef struct 
{
    uint32_t magic;
    uint32_t version;
    uint32_t headersize;
    uint32_t flags;
    uint32_t numglyph;
    uint32_t bytesperglyph;
    uint32_t height;
    uint32_t width;
    uint8_t glyphs;
} __attribute__((packed)) psf2_t;

extern volatile psf2_t _binary_resources_font_psf_start;

void puts(char* s)
{
    int x,y,kx=0,line,mask,offs;
    int bpl=(_binary_resources_font_psf_start.width+7)/8;
    while (*s) 
    {
        uint8_t* glyph = (uint8_t*)&_binary_resources_font_psf_start + _binary_resources_font_psf_start.headersize + (*s > 0 && *s < _binary_resources_font_psf_start.numglyph ? *s : 0) * _binary_resources_font_psf_start.bytesperglyph;
        offs = (kx * (_binary_resources_font_psf_start.width+1) * 4);
        for(y=0;y<_binary_resources_font_psf_start.height;y++) {
            line=offs; mask=1<<(_binary_resources_font_psf_start.width-1);
            for(x=0;x<_binary_resources_font_psf_start.width;x++) {
                *((uint32_t*)((uint64_t)&fb+line))=((int)*glyph) & (mask)?0xFFFFFF:0;
                mask>>=1; line+=4;
            }
            *((uint32_t*)((uint64_t)&fb+line))=0; glyph+=bpl; offs+=bootboot.fb_scanline;
        }
        s++; kx++;
    }
}
