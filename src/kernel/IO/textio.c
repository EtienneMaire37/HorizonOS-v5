#pragma once

void tty_show_cursor(uint8_t scanlineStart, uint8_t scanlineEnd)
{
	outb(0x3d4, 0x0a);
	outb(0x3d5, (inb(0x3d5) & 0xc0) | scanlineStart);
 
	outb(0x3d4, 0x0b);
	outb(0x3d5, (inb(0x3d5) & 0xe0) | scanlineEnd);
}

void tty_hide_cursor()
{
	outb(0x3d4, 0x0a);
	outb(0x3d5, 0x20);
}

void tty_reset_cursor()
{
	tty_show_cursor(14, 15);
}

void tty_set_cursor_pos(uint16_t pos)
{
	outb(0x3d4, 0x0f);
	outb(0x3d5, (pos & 0xff));
	outb(0x3d4, 0x0e);
	outb(0x3d5, ((pos >> 8) & 0xff));
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

void outc(char c)
{
	switch((uint32_t)current_stream)
	{
	case (uint32_t)klog:
		debug_outc(c);
		break;
		
	case (uint32_t)kstdout:
	{
		switch(c)
		{
		case '\n':
			tty_cursor += 80;

		case '\r':
			tty_cursor = (int)(tty_cursor / 80) * 80;

			break;

		case '\b':
			tty_cursor--;
			outc(' ');
			tty_cursor--;

			break;

		case '\t':
		{
			// for(uint8_t i = 0; i < TAB_LENGTH; i++)
			uint8_t first_tab_x = (tty_cursor % 80) / TAB_LENGTH;
			while(first_tab_x == (tty_cursor % 80) / TAB_LENGTH)
				outc(' ');

			break;
		}

		default:
			tty_vram[tty_cursor]._char = c;
			tty_vram[tty_cursor].color = tty_color;

			tty_cursor++;
		}

		break;
	}
	default:
		;
	}

	while((tty_cursor / 80) >= 24)	// Last line
	{
		kmemcpy(&tty_vram[0], &tty_vram[80], 80 * 25 * sizeof(tty_char_t));
		
		for(uint8_t i = 0; i < 80; i++)
		{
			tty_vram[24 * 80 + i]._char = ' ';
			tty_vram[24 * 80 + i].color = FG_WHITE | BG_BLACK;
		}

		tty_cursor -= 80;
	}

	tty_cursor %= 80 * 25;
}