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

static int tty_ansi_sequence_state = 0, tty_ansi_sequence_first_nb = 0;

uint8_t tty_ansi_to_vga(uint8_t ansi_code) 
{
    switch (ansi_code) 
	{
        case 30: return FG_BLACK;
        case 31: return FG_RED;
        case 32: return FG_GREEN;
        case 33: return FG_BROWN;
        case 34: return FG_BLUE;
        case 35: return FG_MAGENTA;
        case 36: return FG_CYAN;
        case 37: return FG_LIGHTGRAY;

        case 40: return BG_BLACK;
        case 41: return BG_RED;
        case 42: return BG_GREEN;
        case 43: return BG_BROWN;
        case 44: return BG_BLUE;
        case 45: return BG_MAGENTA;
        case 46: return BG_CYAN;
        case 47: return BG_LIGHTGRAY;

        case 90: return FG_DARKGRAY;
        case 91: return FG_LIGHTRED;
        case 92: return FG_LIGHTGREEN;
        case 93: return FG_YELLOW;
        case 94: return FG_LIGHTBLUE;
        case 95: return FG_LIGHTMAGENTA;
        case 96: return FG_LIGHTCYAN;
        case 97: return FG_WHITE;

        case 100: return BG_DARKGRAY;
        case 101: return BG_LIGHTRED;
        case 102: return BG_LIGHTGREEN;
        case 103: return BG_YELLOW;
        case 104: return BG_LIGHTBLUE;
        case 105: return BG_LIGHTMAGENTA;
        case 106: return BG_LIGHTCYAN;
        case 107: return BG_WHITE;
    }

	return FG_WHITE | BG_BLACK;
}

uint8_t tty_ansi_to_vga_mask(uint8_t ansi_code) 
{
	if ((ansi_code >= 30 && ansi_code <= 37) || (ansi_code >= 90 && ansi_code <= 97))
		return 0x0f;
	if ((ansi_code >= 40 && ansi_code <= 47) || (ansi_code >= 100 && ansi_code <= 107))
		return 0xf0;
    return 0xff;
}

void tty_ansi_reset_m_state()
{
	tty_ansi_sequence_state = 0;
	tty_ansi_sequence_first_nb = 0;
	tty_set_color(FG_WHITE, BG_BLACK);
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
		// tty_outc(' ');
		// tty_cursor--;

		break;

	case '\t':
	{
		// for(uint8_t i = 0; i < TAB_LENGTH; i++)
		uint8_t first_tab_x = (tty_cursor % 80) / TAB_LENGTH;
		while(first_tab_x == (tty_cursor % 80) / TAB_LENGTH)
			tty_outc(' ');

		break;
	}

	case '\x1b': // * ANSI escape sequence
		tty_ansi_sequence_state = 1;
		tty_ansi_sequence_first_nb = 0;
		break;

	case '[':
		if (tty_ansi_sequence_state == 1)
			tty_ansi_sequence_state++;
		else
		{
			tty_ansi_sequence_state = 0;
			tty_ansi_sequence_first_nb = 0;
			goto default_character;
		}
		break;

	case ';':
		goto default_character;
		break;

	case 'm':
		if (tty_ansi_sequence_state != 0)
		{
			if (tty_ansi_sequence_first_nb == 0)
				tty_ansi_reset_m_state();
			else
			{
				uint8_t code = tty_ansi_to_vga(tty_ansi_sequence_first_nb & 0xff);
				uint8_t mask = tty_ansi_to_vga_mask(tty_ansi_sequence_first_nb & 0xff);
				tty_color = (tty_color & (~mask)) | (code & mask);
				tty_ansi_sequence_state = 0;
			}
			break;
		}
		else
			goto default_character;

	case 'J':
		if (tty_ansi_sequence_state != 0)
		{
			switch(tty_ansi_sequence_first_nb)
			{
			case 2:
				tty_clear_screen(' ');
				break;
			default:
				;
			}
			tty_ansi_sequence_state = 0;
			break;
		}
		else
			goto default_character;

	case 'H':
		if (tty_ansi_sequence_state != 0)
		{
			tty_cursor = 0;
			tty_ansi_sequence_state = 0;
			break;
		}
		else
			goto default_character;

	default:
	default_character:
		if (tty_ansi_sequence_state == 2)
		{
			if (c >= '0' && c <= '9')
			{
				tty_ansi_sequence_first_nb *= 10;
				tty_ansi_sequence_first_nb += c - '0';
			}
			else
				tty_ansi_sequence_state++;
			break;
		}
		tty_vram[tty_cursor]._char = c;
		tty_vram[tty_cursor].color = tty_color;

		tty_cursor++;
	}

	bool scrolled = false;
	while ((tty_cursor / 80) >= 25)	// Last line
	{
		memmove(&tty_vram[0], &tty_vram[80], 80 * 25 * sizeof(tty_char_t));
		
		for(uint8_t i = 0; i < 80; i++)
		{
			tty_vram[24 * 80 + i]._char = ' ';
			tty_vram[24 * 80 + i].color = FG_WHITE | BG_BLACK;
		}

		tty_cursor -= 80;

		scrolled = true;
	}

	tty_cursor %= 80 * 25;

	if (scrolled)
		tty_update_cursor();
}

void tty_set_color(uint8_t fg_color, uint8_t bg_color)
{
	fflush(stdout);

	tty_color = (fg_color & 0x0f) | (bg_color & 0xf0);
}