#pragma once

#include <termios.h>
#include <stdbool.h>
#include "../files/psf.h"
#include "../vga/constants.h"
#include "../multitasking/mutex.h"

static inline bool is_printable_character(char c)
{
    return ((unsigned char)c >= 32) && ((unsigned char)c < 128);
}

typedef uint32_t tty_char_t;

#define TAB_LENGTH       8

#define TTY_ESC_BUFFER     32
#define TTY_OSC_BUFFER     128

#define MAX_TTY_X   1024
#define MAX_TTY_Y   1024

#define TTY_AR      2

#define TTY_BOLD            ((tty_char_t)0x10000)
#define TTY_CONTINUE_CHAR   ((tty_char_t)0x20000)
#define TTY_DIRTY           ((tty_char_t)0x40000)

extern tty_char_t* tty_data;

extern uint32_t tty_dirty;
extern bool tty_dirty_cursor;

extern uint32_t tty_res_x, tty_res_y;

extern int32_t tty_cursor;
extern uint8_t tty_color;

extern struct termios tty_ts;

extern pid_t tty_foreground_pgrp;

extern psf_font_t tty_font, tty_font_bold;

extern bool tty_application_cursor_mode;

static inline void tty_set_dirty(uint32_t cursor)
{
    if (cursor % MAX_TTY_X >= tty_res_x || cursor / MAX_TTY_X >= tty_res_y) return;
    if (!(tty_data[cursor] & TTY_DIRTY))
    {
        if (cursor == (uint32_t)tty_cursor)
            tty_dirty_cursor = true;
        tty_dirty++;
        tty_data[cursor] |= TTY_DIRTY;
    }
}

bool __tty_is_dirty();

void tty_init();
void tty_refresh_screen();
void __tty_refresh_screen();
void tty_clear_screen(char c);
void __tty_clear_screen(char c);
void __tty_clear_section(uint32_t start_char, uint32_t end_char, uint8_t clear_color);
void tty_set_color(uint8_t fg_color, uint8_t bg_color);
void tty_set_window_size(int sx, int sy);
void tty_outc_ex(char c, int flags);
void tty_outc(char c);
void __tty_render_cursor(uint32_t cursor);
void __tty_set_character(uint32_t cursor, tty_char_t c);
void __tty_render_character(uint32_t cursor);
void __tty_move_characters(uint32_t start, int offset);
