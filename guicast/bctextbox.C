
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */

#include "bcclipboard.h"
#include "bclistboxitem.h"
#include "bcresources.h"
#include "bcsignals.h"
#include "bctextbox.h"
#include "clip.h"
#include "colors.h"
#include <ctype.h>
#include "cursors.h"
#include "keys.h"
#include <math.h>
#include "bctimer.h"
#include "vframe.h"

#include <string.h>

#define VERTICAL_MARGIN 2
#define VERTICAL_MARGIN_NOBORDER 0
#define HORIZONTAL_MARGIN 4
#define HORIZONTAL_MARGIN_NOBORDER 2

BC_TextBox::BC_TextBox(int x, 
	int y, 
	int w, 
	int rows, 
	char *text, 
	int has_border, 
	int font)
 : BC_SubWindow(x, y, w, 0, -1)
{
	skip_cursor = 0;
	reset_parameters(rows, has_border, font);
	strcpy(this->text, text);
}

BC_TextBox::BC_TextBox(int x, 
	int y, 
	int w, 
	int rows, 
	int64_t text, 
	int has_border, 
	int font)
 : BC_SubWindow(x, y, w, 0, -1)
{
	skip_cursor = 0;
	reset_parameters(rows, has_border, font);
	sprintf(this->text, "%lld", text);
}

BC_TextBox::BC_TextBox(int x, 
	int y, 
	int w, 
	int rows, 
	float text, 
	int has_border, 
	int font,
	int precision)
 : BC_SubWindow(x, y, w, 0, -1)
{
	skip_cursor = 0;
	this->precision = precision;
	reset_parameters(rows, has_border, font);
	sprintf(this->text, "%0.*f", precision, text);
}

BC_TextBox::BC_TextBox(int x, 
	int y, 
	int w, 
	int rows, 
	int text, 
	int has_border, 
	int font)
 : BC_SubWindow(x, y, w, 0, -1)
{
	skip_cursor = 0;
	reset_parameters(rows, has_border, font);
	sprintf(this->text, "%d", text);
}

BC_TextBox::~BC_TextBox()
{
	if(skip_cursor) delete skip_cursor;
}

int BC_TextBox::reset_parameters(int rows, int has_border, int font)
{
	this->rows = rows;
	this->has_border = has_border;
	this->font = font;
	text_start = 0;
	text_end = 0;
	highlight_letter1 = highlight_letter2 = 0;
	highlight_letter3 = highlight_letter4 = 0;
	ibeam_letter = 0;
	active = 0;
	text_selected = word_selected = 0;
	text_x = 0;
	enabled = 1;
	highlighted = 0;
	precision = 4;
	if (!skip_cursor)
		skip_cursor = new Timer;
	keypress_draw = 1;
	last_keypress = 0;
	separators = 0;
	return 0;
}

int BC_TextBox::initialize()
{
	if (!skip_cursor)
		skip_cursor = new Timer;
	skip_cursor->update();
// Get dimensions
	text_ascent = get_text_ascent(font) + 1;
	text_descent = get_text_descent(font) + 1;
	text_height = text_ascent + text_descent;
	ibeam_letter = strlen(text);
	if(has_border)
	{
		left_margin = right_margin = HORIZONTAL_MARGIN;
		top_margin = bottom_margin = VERTICAL_MARGIN;
	}
	else
	{
		left_margin = right_margin = HORIZONTAL_MARGIN_NOBORDER;
		top_margin = bottom_margin = VERTICAL_MARGIN_NOBORDER;
	}
	h = get_row_h(rows);
	text_x = left_margin;
	text_y = top_margin;
	find_ibeam(0);

// Create the subwindow
	BC_SubWindow::initialize();

	BC_Resources *resources = get_resources();
	if(has_border)
	{
		back_color = resources->text_background;
		high_color = resources->text_background_hi;
	}
	else 
	{
		high_color = resources->text_background_noborder_hi;
		back_color = bg_color;
	}

	draw();
	set_cursor(IBEAM_CURSOR);
	return 0;
}

int BC_TextBox::calculate_h(BC_WindowBase *gui, 
	int font, 
	int has_border,
	int rows)
{
	return rows * (gui->get_text_ascent(font) + 1 + 
		gui->get_text_descent(font) + 1) +
		2 * (has_border ? VERTICAL_MARGIN : VERTICAL_MARGIN_NOBORDER);
}

void BC_TextBox::set_precision(int precision)
{
	this->precision = precision;
}

void BC_TextBox::set_selection(int char1, int char2, int ibeam)
{
	highlight_letter1 = char1;
	highlight_letter2 = char2;
	ibeam_letter = ibeam;
	draw();
}

int BC_TextBox::update(char *text)
{
//printf("BC_TextBox::update 1 %d %s %s\n", strcmp(text, this->text), text, this->text);
	int text_len = strlen(text);
// Don't update if contents are the same
	if(!strcmp(text, this->text)) return 0;


	strcpy(this->text, text);
	if(highlight_letter1 > text_len) highlight_letter1 = text_len;
	if(highlight_letter2 > text_len) highlight_letter2 = text_len;
	if(ibeam_letter > text_len) ibeam_letter = text_len;
	draw();
	return 0;
}

int BC_TextBox::update(int64_t value)
{
	char string[BCTEXTLEN];
	sprintf(string, "%lld", value);


	update(string);
	return 0;
}

int BC_TextBox::update(float value)
{
	char string[BCTEXTLEN];
	sprintf(string, "%0.*f", precision, value);

	update(string);
	return 0;
}

void BC_TextBox::disable()
{
	if(enabled)
	{
		enabled = 0;
		if(top_level)
		{
			if(active) top_level->deactivate();
			draw();
		}
	}
}

void BC_TextBox::enable()
{
	if(!enabled)
	{
		enabled = 1;
		if(top_level)
		{
			draw();
		}
	}
}

int BC_TextBox::get_enabled()
{
	return enabled;
}

int BC_TextBox::pixels_to_rows(BC_WindowBase *window, int font, int pixels)
{
	return (pixels - 4) / 
		(window->get_text_ascent(font) + 1 + 
			window->get_text_descent(font) + 1);
}

int BC_TextBox::calculate_row_h(int rows, 
	BC_WindowBase *parent_window, 
	int has_border, 
	int font)
{
	return rows * 
		(parent_window->get_text_ascent(font) + 1 + 
		parent_window->get_text_descent(font) + 1) +
		(has_border ? 4 : 0);
}

char* BC_TextBox::get_text()
{
	return text;
}

int BC_TextBox::get_text_rows()
{
	int text_len = strlen(text);
	int result = 1;
	for(int i = 0; i < text_len; i++)
	{
		if(text[i] == 0xa) result++;
	}
	return result;
}


int BC_TextBox::get_row_h(int rows)
{
	return rows * text_height + top_margin + bottom_margin;
}

int BC_TextBox::reposition_window(int x, int y, int w, int rows)
{
	int new_h = get_h();
	if(w < 0) w = get_w();
	if(rows != -1)
	{
		new_h = get_row_h(rows);
		this->rows = rows;
	}

	if(x != get_x() || 
		y != get_y() || 
		w != get_w() || 
		new_h != get_h())
	{
// printf("BC_TextBox::reposition_window 1 %d %d %d %d %d %d %d %d\n",
// x, get_x(), y, get_y(), w, get_w(), new_h, get_h());
		BC_WindowBase::reposition_window(x, y, w, new_h);
		draw();
	}
	return 0;
}

void BC_TextBox::draw_border()
{
	BC_Resources *resources = get_resources();
// Clear margins
	set_color(background_color);
	draw_box(0, 0, left_margin, get_h());
	draw_box(get_w() - right_margin, 0, right_margin, get_h());

	if(has_border)
	{
		if(highlighted)
			draw_3d_border(0, 0, w, h,
				resources->button_shadow, 
				resources->button_uphighlighted,
			        resources->button_highlighted,
			        resources->button_light);
		else
			draw_3d_border(0, 0, w, h, 
				resources->text_border1, 
				resources->text_border2,
				resources->text_border3,
				resources->text_border4);
	}
}

void BC_TextBox::draw_cursor()
{
//	set_color(background_color);
	set_color(WHITE);
	set_inverse();

	draw_box(ibeam_x + text_x, 
		ibeam_y + text_y, 
		BCCURSORW, 
		text_height);
	set_opaque();
}


void BC_TextBox::draw()
{
	int i, j, k, text_len;
	int row_begin, row_end;
	int highlight_x1, highlight_x2;
	int need_ibeam = 1;
	BC_Resources *resources = get_resources();

//printf("BC_TextBox::draw 1 %s\n", text);
// Background
	if(has_border)
	{
		background_color = resources->text_background;
	}
	else
	{
		if(highlighted)
		{
			background_color = high_color;
		}
		else
		{
			background_color = back_color;
		}
	}

	set_color(background_color);
	draw_box(0, 0, w, h);

// Draw text with selection
	set_font(font);
	text_len = strlen(text);
//printf("BC_TextBox::draw 0 %s %d %d %d %d\n", text, text_y, text_len, get_w(), text_height);

	for(i = 0, k = text_y; i < text_len && k < get_h(); k += text_height)
	{
// Draw row of text
		if(text[i] == '\n') i++;
		row_begin = i;
		for(j = 0; text[i] != '\n' && i < text_len; j++, i++)
		{
			text_row[j] = text[i];
		}
		row_end = i;
		text_row[j] = 0;

//printf("BC_TextBox::draw 1 %d %d %c\n", row_begin, row_end, text_row[j - 1]);

		if(k > -text_height + top_margin && k < get_h() - bottom_margin)
		{
// Draw highlighted region of row
			if(highlight_letter2 > highlight_letter1 &&
				highlight_letter2 > row_begin && highlight_letter1 < row_end)
			{
				if(active && enabled && get_has_focus())
					set_color(resources->text_highlight);
				else
					set_color(resources->text_inactive_highlight);

				if(highlight_letter1 >= row_begin && highlight_letter1 < row_end)
					highlight_x1 = get_text_width(font, text_row, highlight_letter1 - row_begin);
				else
					highlight_x1 = 0;

				if(highlight_letter2 > row_begin && highlight_letter2 <= row_end)
					highlight_x2 = get_text_width(font, text_row, highlight_letter2 - row_begin);
				else
					highlight_x2 = get_w();

				draw_box(highlight_x1 + text_x, 
					k, 
					highlight_x2 - highlight_x1, 
					text_height);
			}

// Draw text over highlight
			if(enabled)
				set_color(resources->text_default);
			else
				set_color(MEGREY);

			draw_text(text_x, k + text_ascent, text_row);

// Get ibeam location
			if(ibeam_letter >= row_begin && ibeam_letter <= row_end)
			{
				need_ibeam = 0;
				ibeam_y = k - text_y;
				ibeam_x = get_text_width(font, text_row, ibeam_letter - row_begin);
			}
		}
	}

//printf("BC_TextBox::draw 3 %d\n", ibeam_y);
	if(need_ibeam)
	{
		ibeam_x = 0;
		ibeam_y = 0;
	}

//printf("BC_TextBox::draw 4 %d\n", ibeam_y);
// Draw solid cursor
	if (active) 
		draw_cursor();

// Border
	draw_border();
	flash();
	flush();   
}

int BC_TextBox::focus_in_event()
{
	draw();
	return 1;
}

int BC_TextBox::focus_out_event()
{
	draw();
	return 1;
}

int BC_TextBox::cursor_enter_event()
{
	if(top_level->event_win == win && enabled)
	{
		tooltip_done = 0;

		if(!highlighted)
		{
			highlighted = 1;
			draw_border();
			flash();
			flush();   
		}
	}
	return 0;
}

int BC_TextBox::cursor_leave_event()
{
	if(highlighted)
	{
		highlighted = 0;
		draw_border();
		hide_tooltip();
		flash();
		flush();   
	}
	return 0;
}

int BC_TextBox::button_press_event()
{
	if(get_buttonpress() > 2) return 0;

	int cursor_letter = 0;
	int text_len = strlen(text);

	if(!enabled) return 0;

	if(top_level->event_win == win)
	{
		if(!active)
		{
			hide_tooltip();
			top_level->deactivate();
			activate();
		}

		cursor_letter = get_cursor_letter(top_level->cursor_x, top_level->cursor_y);
		if(get_double_click())
		{
			word_selected = 1;
			select_word(highlight_letter1, highlight_letter2, cursor_letter);
			highlight_letter3 = highlight_letter1;
			highlight_letter4 = highlight_letter2;
			ibeam_letter = highlight_letter2;
			copy_selection(PRIMARY_SELECTION);
		}
		else
		if(get_buttonpress() == 2)
		{
			highlight_letter3 = highlight_letter4 = 
				ibeam_letter = highlight_letter1 = 
				highlight_letter2 = cursor_letter;
			paste_selection(PRIMARY_SELECTION);
		}
		else
		{
			text_selected = 1;
			highlight_letter3 = highlight_letter4 = 
				ibeam_letter = highlight_letter1 = 
				highlight_letter2 = cursor_letter;
		}
		
		if(ibeam_letter < 0) ibeam_letter = 0;
		if(ibeam_letter > text_len) ibeam_letter = text_len;
		draw();
		return 1;
	}
	else
	if(active)
	{
		top_level->deactivate();
	}

	return 0;
}

int BC_TextBox::button_release_event()
{
	if(active)
	{
		hide_tooltip();
		if(text_selected || word_selected)
		{
			text_selected = 0;
			word_selected = 0;
		}
	}
	return 0;
}

int BC_TextBox::cursor_motion_event()
{
	int cursor_letter, text_len = strlen(text), letter1, letter2;
	if(active)
	{
		if(text_selected || word_selected)
		{
			cursor_letter = get_cursor_letter(top_level->cursor_x, top_level->cursor_y);
			if(word_selected)
			{
				select_word(letter1, letter2, cursor_letter);
			}
			else
			if(text_selected)
			{
				letter1 = letter2 = cursor_letter;
			}

			if(letter1 <= highlight_letter3)
			{
				highlight_letter1 = letter1;
				highlight_letter2 = highlight_letter4;
				ibeam_letter = letter1;
			}
			else
			if(letter2 >= highlight_letter4)
			{
				highlight_letter2 = letter2;
				highlight_letter1 = highlight_letter3;
				ibeam_letter = letter2;
			}
			
			copy_selection(PRIMARY_SELECTION);
			find_ibeam(1);
			draw();
			return 1;
		}
	}
	return 0;
}

int BC_TextBox::activate()
{
	top_level->active_subwindow = this;
	active = 1;
	draw();
	top_level->set_repeat(top_level->get_resources()->blink_rate);
	return 0;
}

int BC_TextBox::deactivate()
{
	active = 0;
	top_level->unset_repeat(top_level->get_resources()->blink_rate);
	draw();
	return 0;
}

int BC_TextBox::repeat_event(int64_t duration)
{
	int result = 0;

	if(duration == top_level->get_resources()->tooltip_delay &&
		tooltip_text[0] != 0 &&
		highlighted)
	{
		show_tooltip();
		tooltip_done = 1;
		result = 1;
	}
		
	if(duration == top_level->get_resources()->blink_rate && 
		active &&
		get_has_focus())
	{
		if(skip_cursor->get_difference() < duration)
		{
// printf("BC_TextBox::repeat_event 1 %lld %lld\n", 
// skip_cursor->get_difference(), 
// duration);
			return 1;
		}
		draw_cursor();
		flash();
		flush();   
		result = 1;
	}
	return result;
}

void BC_TextBox::default_keypress(int &dispatch_event, int &result)
{
    if((top_level->get_keypress() == RETURN) ||
//		(top_level->get_keypress() > 30 && top_level->get_keypress() < 127))
		(top_level->get_keypress() > 30 && top_level->get_keypress() <= 255))
	{
// Substitute UNIX linefeed
		if(top_level->get_keypress() == RETURN) 
			temp_string[0] = 0xa;
		else
			temp_string[0] = top_level->get_keypress();
		temp_string[1] = 0;
		insert_text(temp_string);
		find_ibeam(1);
		draw();
		dispatch_event = 1;
		result = 1;
	}
}

int BC_TextBox::select_whole_text(int select)
{
	if (select == 1) 
	{
		highlight_letter1 = 0;
		highlight_letter2 = strlen(text);
		text_selected = word_selected = 0;	
		ibeam_letter = highlight_letter1;
		find_ibeam(1);
		if(keypress_draw) draw();
	} else
	if (select == -1)
	{
		ibeam_letter = strlen(text);
		highlight_letter1 = ibeam_letter;
		highlight_letter2 = ibeam_letter;
		text_selected = word_selected = 0;
		find_ibeam(1);
		if(keypress_draw) draw();
	}
	return highlight_letter2 - highlight_letter1;
}

void BC_TextBox::cycle_textboxes(int amout)
{
	top_level->cycle_textboxes(amout);
}

int BC_TextBox::keypress_event()
{
// Result == 2 contents changed
// Result == 1 trapped keypress
// Result == 0 nothing
	int result = 0;
	int text_len;
	int dispatch_event = 0;

	if(!active || !enabled) return 0;

	text_len = strlen(text);
	last_keypress = get_keypress();
	switch(get_keypress())
	{
		case ESC:
			top_level->deactivate();
			result = 0;
			break;

		case RETURN:
			if(rows == 1)
			{
				top_level->deactivate();
				dispatch_event = 1;
				result = 0;
			}
			else
			{
				default_keypress(dispatch_event, result);
			}
			break;
// Handle like a default keypress

		case TAB:
			cycle_textboxes(1);
			result = 1;
			break;

		case LEFTTAB:
			cycle_textboxes(-1);
			result = 1;
			break;

		case LEFT:
			if(ibeam_letter > 0)
			{
				int old_ibeam_letter = ibeam_letter;
// Single character
				if(!ctrl_down())
				{
					ibeam_letter--;
				}
				else
// Word
				{
					ibeam_letter--;
					while(ibeam_letter > 0 && isalnum(text[ibeam_letter - 1]))
						ibeam_letter--;
				}


// Extend selection
				if(top_level->shift_down())
				{
// Initialize highlighting
					if(highlight_letter1 == highlight_letter2)
					{
						highlight_letter1 = ibeam_letter;
						highlight_letter2 = old_ibeam_letter;
					}
					else
// Extend left highlight
					if(highlight_letter1 == old_ibeam_letter)
					{
						highlight_letter1 = ibeam_letter;
					}
					else
// Shrink right highlight
					if(highlight_letter2 == old_ibeam_letter)
					{
						highlight_letter2 = ibeam_letter;
					}
				}
				else
				{
					highlight_letter1 = highlight_letter2 = ibeam_letter;
				}


				find_ibeam(1);
				if(keypress_draw) draw();
			}
			result = 1;
			break;

		case RIGHT:
			if(ibeam_letter < text_len)
			{
				int old_ibeam_letter = ibeam_letter;
// Single character
				if(!ctrl_down())
				{
					ibeam_letter++;
				}
				else
// Word
				{
					while(ibeam_letter < text_len && isalnum(text[ibeam_letter++]))
						;
				}



// Extend selection
				if(top_level->shift_down())
				{
// Initialize highlighting
					if(highlight_letter1 == highlight_letter2)
					{
						highlight_letter1 = old_ibeam_letter;
						highlight_letter2 = ibeam_letter;
					}
					else
// Shrink left highlight
					if(highlight_letter1 == old_ibeam_letter)
					{
						highlight_letter1 = ibeam_letter;
					}
					else
// Expand right highlight
					if(highlight_letter2 == old_ibeam_letter)
					{
						highlight_letter2 = ibeam_letter;
					}
				}
				else
				{
					highlight_letter1 = highlight_letter2 = ibeam_letter;
				}

				find_ibeam(1);
				if(keypress_draw) draw();
			}
			result = 1;
			break;

		case UP:
			if(ibeam_letter > 0)
			{
//printf("BC_TextBox::keypress_event 1 %d %d %d\n", ibeam_x, ibeam_y, ibeam_letter);
				int new_letter = get_cursor_letter(ibeam_x + text_x, 
					ibeam_y + text_y - text_height);
//printf("BC_TextBox::keypress_event 2 %d %d %d\n", ibeam_x, ibeam_y, new_letter);

// Extend selection
				if(top_level->shift_down())
				{
// Initialize highlighting
					if(highlight_letter1 == highlight_letter2)
					{
						highlight_letter1 = new_letter;
						highlight_letter2 = ibeam_letter;
					}
					else
// Expand left highlight
					if(highlight_letter1 == ibeam_letter)
					{
						highlight_letter1 = new_letter;
					}
					else
// Shrink right highlight
					if(highlight_letter2 == ibeam_letter)
					{
						highlight_letter2 = new_letter;
					}
				}
				else
					highlight_letter1 = highlight_letter2 = new_letter;

				if(highlight_letter1 > highlight_letter2)
				{
					int temp = highlight_letter1;
					highlight_letter1 = highlight_letter2;
					highlight_letter2 = temp;
				}
				ibeam_letter = new_letter;

				find_ibeam(1);
				if(keypress_draw) draw();
			}
			result = 1;
			break;

		case PGUP:
			if(ibeam_letter > 0)
			{
				int new_letter = get_cursor_letter(ibeam_x + text_x, 
					ibeam_y + text_y - get_h());

// Extend selection
				if(top_level->shift_down())
				{
// Initialize highlighting
					if(highlight_letter1 == highlight_letter2)
					{
						highlight_letter1 = new_letter;
						highlight_letter2 = ibeam_letter;
					}
					else
// Expand left highlight
					if(highlight_letter1 == ibeam_letter)
					{
						highlight_letter1 = new_letter;
					}
					else
// Shrink right highlight
					if(highlight_letter2 == ibeam_letter)
					{
						highlight_letter2 = new_letter;
					}
				}
				else
					highlight_letter1 = highlight_letter2 = new_letter;

				if(highlight_letter1 > highlight_letter2)
				{
					int temp = highlight_letter1;
					highlight_letter1 = highlight_letter2;
					highlight_letter2 = temp;
				}
				ibeam_letter = new_letter;

				find_ibeam(1);
				if(keypress_draw) draw();
			}
			result = 1;
			break;

		case DOWN:
//			if(ibeam_letter > 0)
			{
// Extend selection
				int new_letter = get_cursor_letter(ibeam_x + text_x, 
					ibeam_y + text_y + text_height);
//printf("BC_TextBox::keypress_event 10 %d\n", new_letter);

				if(top_level->shift_down())
				{
// Initialize highlighting
					if(highlight_letter1 == highlight_letter2)
					{
						highlight_letter1 = new_letter;
						highlight_letter2 = ibeam_letter;
					}
					else
// Shrink left highlight
					if(highlight_letter1 == ibeam_letter)
					{
						highlight_letter1 = new_letter;
					}
					else
// Expand right highlight
					if(highlight_letter2 == ibeam_letter)
					{
						highlight_letter2 = new_letter;
					}
				}
				else
					highlight_letter1 = highlight_letter2 = new_letter;

				if(highlight_letter1 > highlight_letter2)
				{
					int temp = highlight_letter1;
					highlight_letter1 = highlight_letter2;
					highlight_letter2 = temp;
				}
				ibeam_letter = new_letter;

				find_ibeam(1);
				if(keypress_draw) draw();

//printf("BC_TextBox::keypress_event 20 %d\n", ibeam_letter);
			}
			result = 1;
			break;

		case PGDN:
			{
// Extend selection
				int new_letter = get_cursor_letter(ibeam_x + text_x, 
					ibeam_y + text_y + get_h());
//printf("BC_TextBox::keypress_event 10 %d\n", new_letter);

				if(top_level->shift_down())
				{
// Initialize highlighting
					if(highlight_letter1 == highlight_letter2)
					{
						highlight_letter1 = new_letter;
						highlight_letter2 = ibeam_letter;
					}
					else
// Shrink left highlight
					if(highlight_letter1 == ibeam_letter)
					{
						highlight_letter1 = new_letter;
					}
					else
// Expand right highlight
					if(highlight_letter2 == ibeam_letter)
					{
						highlight_letter2 = new_letter;
					}
				}
				else
					highlight_letter1 = highlight_letter2 = new_letter;

				if(highlight_letter1 > highlight_letter2)
				{
					int temp = highlight_letter1;
					highlight_letter1 = highlight_letter2;
					highlight_letter2 = temp;
				}
				ibeam_letter = new_letter;

				find_ibeam(1);
				if(keypress_draw) draw();

//printf("BC_TextBox::keypress_event 20 %d\n", ibeam_letter);
			}
			result = 1;
			break;
		
		case END:
		{
			int old_ibeam_letter = ibeam_letter;

			while(ibeam_letter < text_len && text[ibeam_letter] != '\n')
				ibeam_letter++;

			if(top_level->shift_down())
			{
// Begin selection
				if(highlight_letter1 == highlight_letter2)
				{
					highlight_letter2 = ibeam_letter;
					highlight_letter1 = old_ibeam_letter;
				}
				else
// Shrink selection
				if(highlight_letter1 == old_ibeam_letter)
				{
					highlight_letter1 = highlight_letter2;
					highlight_letter2 = ibeam_letter;
				}
				else
// Extend selection
				if(highlight_letter2 == old_ibeam_letter)
				{
					highlight_letter2 = ibeam_letter;
				}
			}
			else
				highlight_letter1 = highlight_letter2 = ibeam_letter;

			find_ibeam(1);
			if(keypress_draw) draw();
			result = 1;
			break;
		}

		case HOME:
		{
			int old_ibeam_letter = ibeam_letter;

			while(ibeam_letter > 0 && text[ibeam_letter - 1] != '\n')
				ibeam_letter--;

			if(top_level->shift_down())
			{
// Begin selection
				if(highlight_letter1 == highlight_letter2)
				{
					highlight_letter2 = old_ibeam_letter;
					highlight_letter1 = ibeam_letter;
				}
				else
// Extend selection
				if(highlight_letter1 == old_ibeam_letter)
				{
					highlight_letter1 = ibeam_letter;
				}
				else
// Shrink selection
				if(highlight_letter2 == old_ibeam_letter)
				{
					highlight_letter2 = highlight_letter1;
					highlight_letter1 = ibeam_letter;
				}
			}
			else
				highlight_letter1 = highlight_letter2 = ibeam_letter;

			find_ibeam(1);
			if(keypress_draw) draw();
			result = 1;
			break;
		}

    	case BACKSPACE:
			if(highlight_letter1 == highlight_letter2)
			{
				if(ibeam_letter > 0)
				{
					delete_selection(ibeam_letter - 1, ibeam_letter, text_len);
					ibeam_letter--;
				}
			}
			else
			{
				delete_selection(highlight_letter1, highlight_letter2, text_len);
				highlight_letter2 = ibeam_letter = highlight_letter1;
			}

			find_ibeam(1);
			if(keypress_draw) draw();
			dispatch_event = 1;
			result = 1;
    		break;

		case DELETE:
			if(highlight_letter1 == highlight_letter2)
			{
				if(ibeam_letter < text_len)
				{
					delete_selection(ibeam_letter, ibeam_letter + 1, text_len);
				}
			}
			else
			{
				delete_selection(highlight_letter1, highlight_letter2, text_len);
				highlight_letter2 = ibeam_letter = highlight_letter1;
			}
			
			find_ibeam(1);
			if(keypress_draw) draw();
			dispatch_event = 1;
			result = 1;
			break;



		default:
			if(ctrl_down())
			{
				if(get_keypress() == 'c' || get_keypress() == 'C')
				{
					if(highlight_letter1 != highlight_letter2)
					{
						copy_selection(SECONDARY_SELECTION);
						result = 1;
					}
				}
				else
				if(get_keypress() == 'v' || get_keypress() == 'V')
				{
					paste_selection(SECONDARY_SELECTION);
					find_ibeam(1);
					if(keypress_draw) draw();
					dispatch_event = 1;
					result = 1;
				}
				else
				if(get_keypress() == 'x' || get_keypress() == 'X')
				{
					if(highlight_letter1 != highlight_letter2)
					{
						copy_selection(SECONDARY_SELECTION);
						delete_selection(highlight_letter1, highlight_letter2, text_len);
						highlight_letter2 = ibeam_letter = highlight_letter1;
					}

					find_ibeam(1);
					if(keypress_draw) draw();
					dispatch_event = 1;
					result = 1;
				}
				
				break;
			}

			default_keypress(dispatch_event, result);
			break;
	}

	if(dispatch_event) skip_cursor->update();
	if(dispatch_event) handle_event();
//printf("BC_TextBox::keypress_event 100 %d\n", ibeam_letter);
	return result;
}



int BC_TextBox::uses_text()
{
	return 1;
}

void BC_TextBox::delete_selection(int letter1, int letter2, int text_len)
{
	int i, j;
	
	for(i = letter1, j = letter2; j < text_len; i++, j++)
	{
		text[i] = text[j];
	}
	text[i] = 0;

	do_separators(1);
}

void BC_TextBox::insert_text(char *string)
{
	int i, j, text_len, string_len;

	string_len = strlen(string);
	text_len = strlen(text);
	if(highlight_letter1 < highlight_letter2)
	{
		delete_selection(highlight_letter1, highlight_letter2, text_len);
		highlight_letter2 = ibeam_letter = highlight_letter1;
	}

	text_len = strlen(text);

	for(i = text_len, j = text_len + string_len; i >= ibeam_letter; i--, j--)
		text[j] = text[i];

	for(i = ibeam_letter, j = 0; j < string_len; j++, i++)
		text[i] = string[j];

	ibeam_letter += string_len;

	do_separators(0);
}

void BC_TextBox::do_separators(int ibeam_left)
{
	if(separators)
	{
// Remove separators from text
		int text_len = strlen(text);
		int separator_len = strlen(separators);
		for(int i = 0; i < text_len; i++)
		{
			if(!isalnum(text[i]))
			{
				for(int j = i; j < text_len - 1; j++)
					text[j] = text[j + 1];
				if(!ibeam_left && i < ibeam_letter) ibeam_letter--;
				text_len--;
				i--;
			}
		}
		text[text_len] = 0;






// Insert separators into text
		for(int i = 0; i < separator_len; i++)
		{
			if(i < text_len)
			{
// Insert a separator
				if(!isalnum(separators[i]))
				{
					for(int j = text_len; j >= i; j--)
					{
						text[j + 1] = text[j];
					}
					if(!ibeam_left && i < ibeam_letter) ibeam_letter++;
					text_len++;
					text[i] = separators[i];
				}
			}
			else
			if(i >= text_len)
			{
				text[i] = separators[i];
			}
		}

// Truncate text
		text[separator_len] = 0;
	}

}

void BC_TextBox::get_ibeam_position(int &x, int &y)
{
	int i, j, k, row_begin, row_end, text_len;

	text_len = strlen(text);
	y = 0;
	x = 0;
	for(i = 0; i < text_len; )
	{
		row_begin = i;
		for(j = 0; text[i] != '\n' && i < text_len; j++, i++)
		{
			text_row[j] = text[i];
		}

		row_end = i;
		text_row[j] = 0;

		if(ibeam_letter >= row_begin && ibeam_letter <= row_end)
		{
			x = get_text_width(font, text_row, ibeam_letter - row_begin);
//printf("BC_TextBox::get_ibeam_position 9 %d %d\n", x, y);
			return;
		}

		if(text[i] == '\n')
		{
			i++;
			y += text_height;
		}
	}
//printf("BC_TextBox::get_ibeam_position 10 %d %d\n", x, y);

	x = 0;
	return;
}

void BC_TextBox::set_text_row(int row)
{
	text_y = -(row * text_height) + top_margin;
	draw();
}

int BC_TextBox::get_text_row()
{
	return -(text_y - top_margin) / text_height;
}

void BC_TextBox::find_ibeam(int dispatch_event)
{
	int x, y;
	int old_x = text_x, old_y = text_y;

	get_ibeam_position(x, y);

	if(left_margin + text_x + x >= get_w() - right_margin - BCCURSORW)
	{
		text_x = -(x - (get_w() - get_w() / 4)) + left_margin;
		if(text_x > left_margin) text_x = left_margin;
	}
	else
	if(left_margin + text_x + x < left_margin)
	{
		text_x = -(x - (get_w() / 4)) + left_margin;
		if(text_x > left_margin) text_x = left_margin;
	}

	while(y + text_y >= get_h() - text_height - bottom_margin)
	{
		text_y -= text_height;
// 		text_y = -(y - (get_h() / 2)) + top_margin;
// 		if(text_y > top_margin) text_y = top_margin;
	}

	while(y + text_y < top_margin)
	{
		text_y += text_height;
		if(text_y > top_margin) 
		{
			text_y = top_margin;
			break;
		}
	}

	if(dispatch_event && (old_x != text_x || old_y != text_y)) motion_event();
}

int BC_TextBox::get_cursor_letter(int cursor_x, int cursor_y)
{
	int i, j, k, l, row_begin, row_end, text_len, result = 0, done = 0;
	text_len = strlen(text);

	if(cursor_y < text_y)
	{
		result = 0;
		done = 1;
	}

	for(i = 0, k = text_y; i < text_len && !done; k += text_height)
	{
		row_begin = i;
		for(j = 0; text[i] != '\n' && i < text_len; j++, i++)
		{
			text_row[j] = text[i];
		}
		row_end = i;
		text_row[j] = 0;

		if(cursor_y >= k && cursor_y < k + text_height)
		{
			for(j = 0; j <= row_end - row_begin && !done; j++)
			{
				l = get_text_width(font, text_row, j) + text_x;
				if(l > cursor_x)
				{
					result = row_begin + j - 1;
					done = 1;
				}
			}
			if(!done)
			{
				result = row_end;
				done = 1;
			}
		}
		if(text[i] == '\n') i++;
		
		if(i >= text_len && !done)
		{
			result = text_len;
		}
	}
	if(result < 0) result = 0;
	if(result > text_len) result = text_len;
	return result;
}

void BC_TextBox::select_word(int &letter1, int &letter2, int ibeam_letter)
{
	int text_len = strlen(text);
	letter1 = letter2 = ibeam_letter;
	do
	{
		if(isalnum(text[letter1])) letter1--;
	}while(letter1 > 0 && isalnum(text[letter1]));
	if(!isalnum(text[letter1])) letter1++;

	do
	{
		if(isalnum(text[letter2])) letter2++;
	}while(letter2 < text_len && isalnum(text[letter2]));
	if(letter2 < text_len && text[letter2] == ' ') letter2++;

	if(letter1 < 0) letter1 = 0;
	if(letter2 < 0) letter2 = 0;
	if(letter1 > text_len) letter1 = text_len;
	if(letter2 > text_len) letter2 = text_len;
}

void BC_TextBox::copy_selection(int clipboard_num)
{
	int text_len = strlen(text);

	if(highlight_letter1 >= text_len ||
		highlight_letter2 > text_len ||
		highlight_letter1 < 0 ||
		highlight_letter2 < 0 ||
		highlight_letter2 - highlight_letter1 <= 0) return;

	get_clipboard()->to_clipboard(&text[highlight_letter1], 
		highlight_letter2 - highlight_letter1, 
		clipboard_num);
}

void BC_TextBox::paste_selection(int clipboard_num)
{
	int len = get_clipboard()->clipboard_len(clipboard_num);
	if(len)
	{
		char *string = new char[len + 1];
		get_clipboard()->from_clipboard(string, len, clipboard_num);
		insert_text(string);
	}
}

void BC_TextBox::set_keypress_draw(int value)
{
	keypress_draw = value;
}

int BC_TextBox::get_last_keypress()
{
	return last_keypress;
}

int BC_TextBox::get_ibeam_letter()
{
	return ibeam_letter;
}

void BC_TextBox::set_ibeam_letter(int number, int redraw)
{
	this->ibeam_letter = number;
	if(redraw)
	{
		draw();
	}
}

void BC_TextBox::set_separators(char *separators)
{
	this->separators = separators;
}
















BC_ScrollTextBox::BC_ScrollTextBox(BC_WindowBase *parent_window, 
	int x, 
	int y, 
	int w,
	int rows,
	char *default_text)
{
	this->parent_window = parent_window;
	this->x = x;
	this->y = y;
	this->w = w;
	this->rows = rows;
	this->default_text = default_text;
}

BC_ScrollTextBox::~BC_ScrollTextBox()
{
	delete yscroll;
	if(text)
	{
		text->gui = 0;
		delete text;
	}
}

void BC_ScrollTextBox::create_objects()
{
// Must be created first
	parent_window->add_subwindow(text = new BC_ScrollTextBoxText(this));
	parent_window->add_subwindow(yscroll = new BC_ScrollTextBoxYScroll(this));
}

int BC_ScrollTextBox::handle_event()
{
	return 1;
}

int BC_ScrollTextBox::get_x()
{
	return x;
}

int BC_ScrollTextBox::get_y()
{
	return y;
}

int BC_ScrollTextBox::get_w()
{
	return w;
}

int BC_ScrollTextBox::get_rows()
{
	return rows;
}


char* BC_ScrollTextBox::get_text()
{
	return text->get_text();
}

void BC_ScrollTextBox::update(char *text)
{
	this->text->update(text);
	yscroll->update_length(this->text->get_text_rows(),
		this->text->get_text_row(),
		yscroll->get_handlelength());
}

void BC_ScrollTextBox::reposition_window(int x, int y, int w, int rows)
{
	this->x = x;
	this->y = y;
	this->w = w;
	this->rows = rows;
	text->reposition_window(x, 
		y, 
		w - yscroll->get_span(), 
		rows);
	yscroll->reposition_window(x + w - yscroll->get_span(), 
		y, 
		BC_TextBox::calculate_row_h(rows, 
			parent_window));
	yscroll->update_length(text->get_text_rows(),
		text->get_text_row(),
		rows);
}









BC_ScrollTextBoxText::BC_ScrollTextBoxText(BC_ScrollTextBox *gui)
 : BC_TextBox(gui->x, 
 	gui->y, 
	gui->w - get_resources()->vscroll_data[SCROLL_HANDLE_UP]->get_w(), 
	gui->rows,
	gui->default_text)
{
	this->gui = gui;
}

BC_ScrollTextBoxText::~BC_ScrollTextBoxText()
{
	if(gui)
	{
		gui->text = 0;
		delete gui;
	}
}

int BC_ScrollTextBoxText::handle_event()
{
	gui->yscroll->update_length(get_text_rows(),
		get_text_row(),
		gui->yscroll->get_handlelength());
	return gui->handle_event();
}

int BC_ScrollTextBoxText::motion_event()
{
	gui->yscroll->update_length(get_text_rows(),
		get_text_row(),
		gui->yscroll->get_handlelength());
	return 1;
}


BC_ScrollTextBoxYScroll::BC_ScrollTextBoxYScroll(BC_ScrollTextBox *gui)
 : BC_ScrollBar(gui->x + 
 			gui->w - 
			get_resources()->vscroll_data[SCROLL_HANDLE_UP]->get_w(), 
		gui->y, 
		SCROLL_VERT, 
		BC_TextBox::calculate_row_h(gui->rows, 
			gui->parent_window), 
		gui->text->get_text_rows(), 
		0, 
		gui->rows)
{
	this->gui = gui;
}

BC_ScrollTextBoxYScroll::~BC_ScrollTextBoxYScroll()
{
}

int BC_ScrollTextBoxYScroll::handle_event()
{
	gui->text->set_text_row(get_position());
	return 1;
}










BC_PopupTextBoxText::BC_PopupTextBoxText(BC_PopupTextBox *popup, int x, int y)
 : BC_TextBox(x, y, popup->text_w, 1, popup->default_text)
{
	this->popup = popup;
}

BC_PopupTextBoxText::~BC_PopupTextBoxText()
{
	if(popup)
	{
		popup->textbox = 0;
		delete popup;
		popup = 0;
	}
}


int BC_PopupTextBoxText::handle_event()
{
	popup->handle_event();
	return 1;
}

BC_PopupTextBoxList::BC_PopupTextBoxList(BC_PopupTextBox *popup, int x, int y)
 : BC_ListBox(x,
 	y,
	popup->text_w + BC_WindowBase::get_resources()->listbox_button[0]->get_w(),
	popup->list_h,
	LISTBOX_TEXT,
	popup->list_items,
	0,
	0,
	1,
	0,
	1)
{
	this->popup = popup;
}
int BC_PopupTextBoxList::handle_event()
{
	BC_ListBoxItem *item = get_selection(0, 0);
	if(item)
	{
		popup->textbox->update(item->get_text());
		popup->handle_event();
	}
	return 1;
}




BC_PopupTextBox::BC_PopupTextBox(BC_WindowBase *parent_window, 
		ArrayList<BC_ListBoxItem*> *list_items,
		char *default_text,
		int x, 
		int y, 
		int text_w,
		int list_h)
{
	this->x = x;
	this->y = y;
	this->list_h = list_h;
	this->default_text = default_text;
	this->text_w = text_w;
	this->parent_window = parent_window;
	this->list_items = list_items;
}

BC_PopupTextBox::~BC_PopupTextBox()
{
	delete listbox;
	if(textbox) 
	{
		textbox->popup = 0;
		delete textbox;
	}
}

int BC_PopupTextBox::create_objects()
{
	int x = this->x, y = this->y;
	parent_window->add_subwindow(textbox = new BC_PopupTextBoxText(this, x, y));
	x += textbox->get_w();
	parent_window->add_subwindow(listbox = new BC_PopupTextBoxList(this, x, y));
	return 0;
}

void BC_PopupTextBox::update(char *text)
{
	textbox->update(text);
}

void BC_PopupTextBox::update_list(ArrayList<BC_ListBoxItem*> *data)
{
	listbox->update(data, 
		0, 
		0,
		1);
}


char* BC_PopupTextBox::get_text()
{
	return textbox->get_text();
}

int BC_PopupTextBox::get_number()
{
	return listbox->get_selection_number(0, 0);
}

int BC_PopupTextBox::get_x()
{
	return x;
}

int BC_PopupTextBox::get_y()
{
	return y;
}

int BC_PopupTextBox::get_w()
{
	return textbox->get_w() + listbox->get_w();
}

int BC_PopupTextBox::get_h()
{
	return textbox->get_h();
}

int BC_PopupTextBox::handle_event()
{
	return 1;
}

int BC_PopupTextBox::reposition_widget(int x, int y, int w, int h) {
	reposition_window(x, y);
	return(0);
}

void BC_PopupTextBox::reposition_window(int x, int y)
{
	this->x = x;
	this->y = y;
	int x1 = x, y1 = y;
	textbox->reposition_window(x1, y1);
	x1 += textbox->get_w();
	listbox->reposition_window(x1, y1);
}














BC_TumbleTextBoxText::BC_TumbleTextBoxText(BC_TumbleTextBox *popup, 
	int64_t default_value,
	int64_t min,
	int64_t max,
	int x, 
	int y)
 : BC_TextBox(x, 
 	y, 
	popup->text_w, 
	1, 
	default_value)
{
	this->popup = popup;
}

BC_TumbleTextBoxText::BC_TumbleTextBoxText(BC_TumbleTextBox *popup, 
	float default_value,
	float min,
	float max,
	int x, 
	int y)
 : BC_TextBox(x, 
 	y, 
	popup->text_w, 
	1, 
	default_value)
{
	this->popup = popup;
}

BC_TumbleTextBoxText::~BC_TumbleTextBoxText()
{
	if(popup)
	{
		popup->textbox = 0;
		delete popup;
		popup = 0;
	}
}



int BC_TumbleTextBoxText::handle_event()
{
	popup->handle_event();
	return 1;
}

int BC_TumbleTextBoxText::button_press_event()
{
	if(is_event_win())
	{
		if(get_buttonpress() < 4) return BC_TextBox::button_press_event();
		if(get_buttonpress() == 4)
		{
			popup->tumbler->handle_up_event();
		}
		else
		if(get_buttonpress() == 5)
		{
			popup->tumbler->handle_down_event();
		}
		return 1;
	}
	return 0;
}




BC_TumbleTextBox::BC_TumbleTextBox(BC_WindowBase *parent_window, 
		int64_t default_value,
		int64_t min,
		int64_t max,
		int x, 
		int y, 
		int text_w)
{
	reset();
	this->x = x;
	this->y = y;
	this->min = min;
	this->max = max;
	this->default_value = default_value;
	this->text_w = text_w;
	this->parent_window = parent_window;
	use_float = 0;
	precision = 4;
	increment = 1;
	log_floatincrement = 0;
}

BC_TumbleTextBox::BC_TumbleTextBox(BC_WindowBase *parent_window, 
		int default_value,
		int min,
		int max,
		int x, 
		int y, 
		int text_w)
{
	reset();
	this->x = x;
	this->y = y;
	this->min = min;
	this->max = max;
	this->default_value = default_value;
	this->text_w = text_w;
	this->parent_window = parent_window;
	use_float = 0;
	precision = 4;
	increment = 1;
	log_floatincrement = 0;
}

BC_TumbleTextBox::BC_TumbleTextBox(BC_WindowBase *parent_window, 
		float default_value_f,
		float min_f,
		float max_f,
		int x, 
		int y, 
		int text_w)
{
	reset();
	this->x = x;
	this->y = y;
	this->min_f = min_f;
	this->max_f = max_f;
	this->default_value_f = default_value_f;
	this->text_w = text_w;
	this->parent_window = parent_window;
	use_float = 1;
	precision = 4;
	increment = 1;
	log_floatincrement = 0;
}

BC_TumbleTextBox::~BC_TumbleTextBox()
{
// Recursive delete.  Normally ~BC_TumbleTextBox is never called but textbox
// is deleted anyway by the windowbase so textbox deletes this.
	if(tumbler) delete tumbler;
	tumbler = 0;
// Don't delete text here if we were called by ~BC_TumbleTextBoxText
	if(textbox)
	{
		textbox->popup = 0;
		delete textbox;
	}
	textbox = 0;
}

void BC_TumbleTextBox::reset()
{
	textbox = 0;
	tumbler = 0;
	increment = 1.0;
	log_floatincrement = 0;
}

void BC_TumbleTextBox::set_precision(int precision)
{
	this->precision = precision;
}

void BC_TumbleTextBox::set_increment(float value)
{
	this->increment = value;
	if(tumbler) tumbler->set_increment(value);
}

void BC_TumbleTextBox::set_log_floatincrement(int value)
{
	this->log_floatincrement = value;
	if(tumbler) tumbler->set_log_floatincrement(value);
}

int BC_TumbleTextBox::create_objects()
{
	int x = this->x, y = this->y;

	if(use_float)
	{
		parent_window->add_subwindow(textbox = new BC_TumbleTextBoxText(this, 
			default_value_f,
			min_f, 
			max_f, 
			x, 
			y));
		textbox->set_precision(precision);
	}
	else
		parent_window->add_subwindow(textbox = new BC_TumbleTextBoxText(this, 
			default_value,
			min, 
			max, 
			x, 
			y));

	x += textbox->get_w();

	if(use_float)
		parent_window->add_subwindow(tumbler = new BC_FTumbler(textbox, 
 			min_f,
			max_f,
			x, 
			y));
	else
		parent_window->add_subwindow(tumbler = new BC_ITumbler(textbox, 
 			min, 
			max, 
			x, 
			y));

	tumbler->set_increment(increment);
	tumbler->set_log_floatincrement(log_floatincrement);
	return 0;
}

char* BC_TumbleTextBox::get_text()
{
	return textbox->get_text();
}

int BC_TumbleTextBox::update(char *value)
{
	textbox->update(value);
	return 0;
}

int BC_TumbleTextBox::update(int64_t value)
{
	textbox->update(value);
	return 0;
}

int BC_TumbleTextBox::update(float value)
{
	textbox->update(value);
	return 0;
}


int BC_TumbleTextBox::get_x()
{
	return x;
}

int BC_TumbleTextBox::get_y()
{
	return y;
}

int BC_TumbleTextBox::get_w()
{
	return textbox->get_w() + tumbler->get_w();
}

int BC_TumbleTextBox::get_h()
{
	return textbox->get_h();
}

int BC_TumbleTextBox::handle_event()
{
	return 1;
}

int BC_TumbleTextBox::reposition_widget(int x, int y, int w, int h) {
	reposition_window(x, y);
	return(0);
}

void BC_TumbleTextBox::reposition_window(int x, int y)
{
	this->x = x;
	this->y = y;
	
	textbox->reposition_window(x, 
 		y, 
		text_w, 
		1);
	tumbler->reposition_window(x + textbox->get_w(),
		y);
}


void BC_TumbleTextBox::set_boundaries(int64_t min, int64_t max)
{
	tumbler->set_boundaries(min, max);
}

void BC_TumbleTextBox::set_boundaries(float min, float max)
{
	tumbler->set_boundaries(min, max);
}
