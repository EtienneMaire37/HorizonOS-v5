#pragma once

#include "textio.h"
#include "vga.h"

static inline void tty_show_cursor(uint8_t scanline_start, uint8_t scanline_end)
{
// // ~ bit 6,7 : reserved; bit 5 : (0: show, 1: hide); bit 0-4: scanline_start
// 	vga_write_port_3d4(VGA_REG_3D4_CURSOR_START, (vga_read_port_3d4(VGA_REG_3D4_CURSOR_START) & 0b11000000) | scanline_start);
// // ~ bit 7 : reserved; bit 5,6 : offset (in chars); bit 0-4: scanline_end
// 	vga_write_port_3d4(VGA_REG_3D4_CURSOR_END, (vga_read_port_3d4(VGA_REG_3D4_CURSOR_END) & 0b10000000) | scanline_end);
}

static inline void tty_hide_cursor()
{
	// vga_write_port_3d4(VGA_REG_3D4_CURSOR_START, vga_read_port_3d4(VGA_REG_3D4_CURSOR_START) | (1 << 5));
}

static inline void tty_reset_cursor()
{
	// tty_show_cursor(14, 15);
}

static inline void tty_set_cursor_pos(uint16_t pos)
{
	// vga_write_port_3d4(VGA_REG_3D4_CURSOR_LOCATION_LOW, pos & 0xff);
	// vga_write_port_3d4(VGA_REG_3D4_CURSOR_LOCATION_HIGH, (pos >> 8) & 0xff);
}

static inline void tty_update_cursor()
{
	tty_set_cursor_pos(tty_cursor);
}

static inline void tty_clear_screen(char c)
{
	for (uint32_t i = 0; i < TTY_RES_X * TTY_RES_Y; i++)
		tty_data[i] = 0x0f20;
	if (c == 0 || c == ' ')
	{
		srgb_t bg_color = vga_get_bg_color(tty_color);
		framebuffer_fill_rect(&framebuffer, 0, 0, framebuffer.width, framebuffer.height, bg_color.r, bg_color.g, bg_color.b, 0);
		tty_cursor = 0;
		goto end;
	}
	tty_cursor = 0;
	for (uint32_t i = 0; i < TTY_RES_X * TTY_RES_Y; i++)
		tty_outc(c);
	tty_cursor = 0;
end:
	// tty_update_cursor();
}

// uint8_t tty_ansi_to_vga(uint8_t ansi_code) 
// {
//     switch (ansi_code) 
// 	{
//         case 30: return FG_BLACK;
//         case 31: return FG_RED;
//         case 32: return FG_GREEN;
//         case 33: return FG_BROWN;
//         case 34: return FG_BLUE;
//         case 35: return FG_MAGENTA;
//         case 36: return FG_CYAN;
//         case 37: return FG_LIGHTGRAY;

//         case 40: return BG_BLACK;
//         case 41: return BG_RED;
//         case 42: return BG_GREEN;
//         case 43: return BG_BROWN;
//         case 44: return BG_BLUE;
//         case 45: return BG_MAGENTA;
//         case 46: return BG_CYAN;
//         case 47: return BG_LIGHTGRAY;

//         case 90: return FG_DARKGRAY;
//         case 91: return FG_LIGHTRED;
//         case 92: return FG_LIGHTGREEN;
//         case 93: return FG_YELLOW;
//         case 94: return FG_LIGHTBLUE;
//         case 95: return FG_LIGHTMAGENTA;
//         case 96: return FG_LIGHTCYAN;
//         case 97: return FG_WHITE;

//         case 100: return BG_DARKGRAY;
//         case 101: return BG_LIGHTRED;
//         case 102: return BG_LIGHTGREEN;
//         case 103: return BG_YELLOW;
//         case 104: return BG_LIGHTBLUE;
//         case 105: return BG_LIGHTMAGENTA;
//         case 106: return BG_LIGHTCYAN;
//         case 107: return BG_WHITE;
//     }

// 	return FG_WHITE | BG_BLACK;
// }

// uint8_t tty_ansi_to_vga_mask(uint8_t ansi_code) 
// {
// 	if ((ansi_code >= 30 && ansi_code <= 37) || (ansi_code >= 90 && ansi_code <= 97))
// 		return 0x0f;
// 	if ((ansi_code >= 40 && ansi_code <= 47) || (ansi_code >= 100 && ansi_code <= 107))
// 		return 0xf0;
//     return 0xff;
// }

static inline void tty_set_color(uint8_t fg_color, uint8_t bg_color)
{
	fflush(stdout);

	tty_color = (fg_color & 0x0f) | (bg_color & 0xf0);
}

static inline uint32_t tty_get_character_width()
{
	return (framebuffer.width - 2 * tty_padding) / TTY_RES_X;
}

static inline uint32_t tty_get_character_height()
{
	return (framebuffer.height - 2 * tty_padding) * (psf_get_glyph_height(&tty_font) + psf_get_glyph_width(&tty_font) - 1) / psf_get_glyph_width(&tty_font) / TTY_RES_X;
}

static inline uint32_t tty_get_character_pos_x(uint32_t index)
{
	return tty_padding + tty_get_character_width() * (index % TTY_RES_X);
}

static inline uint32_t tty_get_character_pos_y(uint32_t index)
{
	return tty_padding + tty_get_character_height() * (index / TTY_RES_X);
}

static inline void tty_render_character(uint32_t cursor, char c, uint8_t color)
{
	if (cursor >= TTY_RES_X * TTY_RES_Y) return;
	
	uint32_t width = tty_get_character_width();
	uint32_t height = tty_get_character_height();

	uint32_t x = tty_get_character_pos_x(cursor);
	uint32_t y = tty_get_character_pos_y(cursor);

	srgb_t bg_color = vga_get_bg_color(color);
	framebuffer_fill_rect(&framebuffer, x, y, width, height, bg_color.r, bg_color.g, bg_color.b, 0);

	if (!is_printable_character(c)) return;

	srgb_t fg_color = vga_get_fg_color(color);
	framebuffer_render_psf2_char(&framebuffer, x, y, width, height, &tty_font, c,
		fg_color.r,
		fg_color.g,
		fg_color.b);
}

static inline void tty_render_cursor(uint32_t cursor)
{
	if (cursor >= TTY_RES_X * TTY_RES_Y) return;

	uint32_t width = tty_get_character_width();
	uint32_t height = tty_get_character_height();

	uint32_t x = tty_get_character_pos_x(cursor);
	uint32_t y = tty_get_character_pos_y(cursor);

	framebuffer_fill_rect(&framebuffer, x, y + (8 * height / 10), width, height / 5, 255, 255, 255, 0);
}

static inline void tty_refresh_screen()
{
	for (uint32_t i = 0; i < TTY_RES_X * TTY_RES_Y; i++)
		tty_render_character(i, tty_data[i], tty_data[i] >> 8);
	if (tty_cursor_blink)
		tty_render_cursor(tty_cursor);
}

static inline void tty_outc(char c)
{
	if (c == 0)
		return;

	if (tty_cursor >= TTY_RES_X * TTY_RES_Y)
	{
		tty_cursor++;
		return;
	}

	// if (is_printable_character(c))
		tty_data[tty_cursor] = c | ((uint16_t)tty_color << 8);

	tty_render_character(tty_cursor, tty_data[tty_cursor], tty_data[tty_cursor] >> 8);
	
	switch(c)
	{
	case '\n':
		tty_cursor += TTY_RES_X;
	case '\r':
		tty_cursor /= TTY_RES_X;
		tty_cursor *= TTY_RES_X;
		break;

	case '\t':
		tty_render_character(tty_cursor, c, tty_color);
		tty_cursor += TAB_LENGTH;
		tty_cursor /= TAB_LENGTH;
		tty_cursor *= TAB_LENGTH;
		if (tty_cursor_blink)
			tty_render_cursor(tty_cursor);
		break;

	case '\b':
		tty_render_character(tty_cursor, c, tty_color);
		if (tty_cursor > 0)
			tty_cursor--;
		if (tty_cursor_blink)
			tty_render_cursor(tty_cursor);
		break;
	
	default:
		if (tty_cursor / TTY_RES_X >= TTY_RES_Y)
		{
			tty_cursor++;
			break;
		}

		tty_cursor++;
	}

	if (tty_cursor_blink)
		tty_render_cursor(tty_cursor);

	while (tty_cursor / TTY_RES_X >= TTY_RES_Y)
	{
		uint32_t rows_to_scroll = tty_cursor / TTY_RES_X - TTY_RES_Y + 1;

		tty_cursor -= rows_to_scroll * TTY_RES_X;

		if (rows_to_scroll >= TTY_RES_Y)
		{
			memset(&tty_data, 0x0f, sizeof(tty_data));
			tty_refresh_screen();
			tty_cursor = (TTY_RES_Y - 1) * TTY_RES_X;
			break;
		}

		for (uint32_t i = 0; i < TTY_RES_Y - rows_to_scroll; i++)
			memcpy(&tty_data[i * TTY_RES_X], &tty_data[(i + rows_to_scroll) * TTY_RES_X], TTY_RES_X * sizeof(uint16_t));

		memset(&tty_data[(TTY_RES_Y - rows_to_scroll) * TTY_RES_X], 0x0f, TTY_RES_X * sizeof(uint16_t));

		tty_refresh_screen();
	}
}