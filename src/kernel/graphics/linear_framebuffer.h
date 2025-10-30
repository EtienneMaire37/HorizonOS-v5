#pragma once

#include "../files/psf.h"

typedef struct linear_framebuffer
{
    uintptr_t address;
    uint32_t width, height;
    uint32_t stride;

    // * From bootboot.h
    uint8_t format;
} linear_framebuffer_t;

linear_framebuffer_t framebuffer;

uint32_t framebuffer_decode_color_data(linear_framebuffer_t* buffer, uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha)
{
    if (!buffer)
        return 0;
    if (!buffer->address)
        return 0;

    switch (buffer->format)
    {
    case FB_ARGB:
        return ((uint32_t)alpha << 24) | ((uint32_t)red << 16) | ((uint32_t)green << 8) | blue;
    case FB_RGBA:
        return ((uint32_t)red << 24) | ((uint32_t)green << 16) | ((uint32_t)blue << 8) | alpha;
    case FB_ABGR:
        return ((uint32_t)alpha << 24) | ((uint32_t)blue << 16) | ((uint32_t)green << 8) | red;
    case FB_BGRA:
        return ((uint32_t)blue << 24) | ((uint32_t)green << 16) | ((uint32_t)red << 8) | alpha;
    }

    return 0;
}

void framebuffer_setpixel(linear_framebuffer_t* buffer, uint32_t x, uint32_t y, uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha)
{
    if (!buffer)
        return;
    if (!buffer->address)
        return;

    if (x >= buffer->width) return;
    if (y >= buffer->height) return;

    ((uint32_t*)(buffer->address + buffer->stride * y))[x] = framebuffer_decode_color_data(buffer, red, green, blue, alpha);
}

void framebuffer_fill_rect(linear_framebuffer_t* buffer, uint32_t x, uint32_t y, uint32_t size_x, uint32_t size_y, uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha)
{
    if (!buffer)
        return;
    if (!buffer->address)
        return;

    if (x >= buffer->width) return;
    if (y >= buffer->height) return;

    uint32_t right = x + size_x, bottom = y + size_y;

    if (right >= buffer->width)
        right = buffer->width;
    if (bottom >= buffer->height)
        bottom = buffer->height;

    uint32_t dword = framebuffer_decode_color_data(buffer, red, green, blue, alpha);

    for (uint32_t i = y; i < bottom; i++)
    {
        for (uint32_t j = x; j < right; j++)
        {
            ((uint32_t*)(buffer->address + buffer->stride * i))[j] = dword;
        }
    }
}

void framebuffer_render_psf2_char(linear_framebuffer_t* buffer, uint32_t x, uint32_t y, uint32_t width, uint32_t height, psf_font_t* font, char c)
{
    if (!font) return;
    if (!font->f) return;
    if (!font->f->data) return;

    void* glyph_data = (void*)psf_get_glyph_data(font);

    uint32_t glyph_index = (uint8_t)c;
    if (glyph_index >= psf_get_num_glyph(font)) return;

    const uint32_t  glyph_width = psf_get_glyph_width(font),
                    glyph_height = psf_get_glyph_height(font);

    uint8_t bytes_per_row = (glyph_width + 7) / 8;
    uint8_t* glyph = (uint8_t*)psf_get_glyph_data(font) + psf_get_bytes_per_glyph(font) * glyph_index;

    for (uint32_t i = y; i < y + height; i++) 
    {
        uint32_t offset_y = (i - y) * glyph_height / height;
        uint8_t* glyph_row = glyph + bytes_per_row * offset_y;

        for (uint32_t j = x; j < x + width; j++) 
        {
            uint32_t offset_x = (j - x) * glyph_width / width;
            uint32_t byte_index = offset_x / 8;
            uint8_t bit_index = 7 - (offset_x % 8);
            bool put = (glyph_row[byte_index] >> bit_index) & 1;

            if (put)
                framebuffer_setpixel(buffer, j, i, 255, 255, 255, 0);
        }
    }
}
