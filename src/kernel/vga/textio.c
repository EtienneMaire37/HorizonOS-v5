#pragma once

void tty_show_cursor(uint8_t scanline_start, uint8_t scanline_end)
{
// ~ bit 6,7 : reserved; bit 5 : (0: show, 1: hide); bit 0-4: scanline_start
	vga_write_port_3d4(VGA_REG_3D4_CURSOR_START, (vga_read_port_3d4(VGA_REG_3D4_CURSOR_START) & 0b11000000) | scanline_start);
// ~ bit 7 : reserved; bit 5,6 : offset (in chars); bit 0-4: scanline_end
	vga_write_port_3d4(VGA_REG_3D4_CURSOR_END, (vga_read_port_3d4(VGA_REG_3D4_CURSOR_END) & 0b10000000) | scanline_end);
}

void tty_hide_cursor()
{
	vga_write_port_3d4(VGA_REG_3D4_CURSOR_START, vga_read_port_3d4(VGA_REG_3D4_CURSOR_START) | (1 << 5));
}

void tty_reset_cursor()
{
	tty_show_cursor(14, 15);
}

void tty_set_cursor_pos(uint16_t pos)
{
	vga_write_port_3d4(VGA_REG_3D4_CURSOR_LOCATION_LOW, pos & 0xff);
	vga_write_port_3d4(VGA_REG_3D4_CURSOR_LOCATION_HIGH, (pos >> 8) & 0xff);
}

void tty_update_cursor()
{
	// tty_cursor %= 80 * 25;
	tty_set_cursor_pos(tty_cursor);
}

void tty_clear_screen(char c)
{
	for(uint16_t i = 0; i < 80 * 25; i++)
	{
        tty_vram[i]._char = c;
		tty_vram[i].color = tty_color;
	}
}

void tty_outc(char c)
{
	if (c == 0) return;
	
	switch(c)
	{
	case '\n':
		tty_cursor += 80;

	case '\r':
		tty_cursor = (int)(tty_cursor / 80) * 80;
		break;

	case '\b':
		tty_cursor--;
		tty_outc(' ');
		tty_cursor--;

		break;

	case '\t':
	{
		// for(uint8_t i = 0; i < TAB_LENGTH; i++)
		uint8_t first_tab_x = (tty_cursor % 80) / TAB_LENGTH;
		while(first_tab_x == (tty_cursor % 80) / TAB_LENGTH)
			tty_outc(' ');

		break;
	}

	default:
		tty_vram[tty_cursor]._char = c;
		tty_vram[tty_cursor].color = tty_color;

		tty_cursor++;
	}

	while ((tty_cursor / 80) >= 25)	// Last line
	{
		memcpy(&tty_vram[0], &tty_vram[80], 80 * 25 * sizeof(tty_char_t));
		
		for(uint8_t i = 0; i < 80; i++)
		{
			tty_vram[24 * 80 + i]._char = ' ';
			tty_vram[24 * 80 + i].color = FG_WHITE | BG_BLACK;
		}

		tty_cursor -= 80;
	}

	tty_cursor %= 80 * 25;
}

void tty_set_color(uint8_t fg_color, uint8_t bg_color)
{
	fflush(stdout);

	tty_color = (fg_color & 0x0f) | (bg_color & 0x70);	// ~ Block blinking
}