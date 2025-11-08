#pragma once

#include "../files/psf.h"

// VGA text mode 3 colors
#define FG_BLACK         0x00
#define FG_BLUE          0x01
#define FG_GREEN         0x02
#define FG_CYAN          0x03
#define FG_RED           0x04
#define FG_MAGENTA       0x05
#define FG_BROWN         0x06
#define FG_LIGHTGRAY     0x07
#define FG_DARKGRAY      0x08
#define FG_LIGHTBLUE     0x09
#define FG_LIGHTGREEN    0x0a
#define FG_LIGHTCYAN     0x0b
#define FG_LIGHTRED      0x0c
#define FG_LIGHTMAGENTA  0x0d
#define FG_YELLOW        0x0e
#define FG_WHITE         0x0f
 
#define BG_BLACK         0x00
#define BG_BLUE          0x10
#define BG_GREEN         0x20
#define BG_CYAN          0x30
#define BG_RED           0x40
#define BG_MAGENTA       0x50
#define BG_BROWN         0x60
#define BG_LIGHTGRAY     0x70

// * Those have the blink attribute on (or high intensity if 3c0.10.3 (normal mode) = 1)
#define BG_DARKGRAY      0x80
#define BG_LIGHTBLUE     0x90
#define BG_LIGHTGREEN    0xa0
#define BG_LIGHTCYAN     0xb0
#define BG_LIGHTRED      0xc0
#define BG_LIGHTMAGENTA  0xd0
#define BG_YELLOW        0xe0
#define BG_WHITE         0xf0

#define TAB_LENGTH       4

#define TTY_RES_X   100

#define TTY_RES_Y   36
// Assume ar (probably should set a max value and dynamically scroll if below a certain threshold)

uint16_t tty_data[TTY_RES_X * TTY_RES_Y] = {0};

uint32_t tty_cursor = 0;
uint8_t tty_color = (FG_WHITE | BG_BLACK);

bool tty_cursor_blink = true;

psf_font_t tty_font;

const uint32_t tty_padding = 2;	// pixels

static inline bool is_printable_character(char c)
{
    return c >= 32;
}

static inline void tty_show_cursor(uint8_t scanlineStart, uint8_t scanlineEnd);
static inline void tty_hide_cursor();
static inline void tty_reset_cursor();
static inline void tty_set_cursor_pos(uint16_t pos);
static inline void tty_clear_screen(char c);
static inline void tty_set_color(uint8_t fg_color, uint8_t bg_color);
static inline void tty_outc(char c);