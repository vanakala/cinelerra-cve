#include "bcclipboard.h"
#include "bclistboxitem.h"
#include "bcresources.h"
#include "bctextbox.h"
#include "colors.h"
#include <ctype.h>
#include "cursors.h"
#include "keys.h"
#include "vframe.h"

#include <string.h>

BC_TextBox::BC_TextBox(int x, int y, int w, int rows, char *text, int has_border, int font)
 : BC_SubWindow(x, y, w, 0, -1)
{
	reset_parameters(rows, has_border, font);
	strcpy(this->text, text);
}

BC_TextBox::BC_TextBox(int x, int y, int w, int rows, long text, int has_border, int font)
 : BC_SubWindow(x, y, w, 0, -1)
{
	reset_parameters(rows, has_border, font);
	sprintf(this->text, "%ld", text);
}

BC_TextBox::BC_TextBox(int x, int y, int w, int rows, float text, int has_border, int font)
 : BC_SubWindow(x, y, w, 0, -1)
{
	reset_parameters(rows, has_border, font);
	sprintf(this->text, "%0.4f", text);
}

BC_TextBox::BC_TextBox(int x, int y, int w, int rows, int text, int has_border, int font)
 : BC_SubWindow(x, y, w, 0, -1)
{
	reset_parameters(rows, has_border, font);
	sprintf(this->text, "%d", text);
}

BC_TextBox::~BC_TextBox()
{
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
	return 0;
}

int BC_TextBox::initialize()
{
// Get dimensions
	text_ascent = get_text_ascent(font) + 1;
	text_descent = get_text_descent(font) + 1;
	text_height = text_ascent + text_descent;
	ibeam_letter = strlen(text);
	if(has_border)
	{ 
		left_margin = right_margin = 4;
		top_margin = bottom_margin = 2;
	}
	else 
	{ 
		left_margin = right_margin = 2;
		top_margin = bottom_margin = 0;
	}
	h = get_row_h(rows);
	text_x = left_margin;
	text_y = top_margin;
	find_ibeam(0);

// Create the subwindow
	BC_SubWindow::initialize();

	if(has_border)
	{
		back_color = WHITE;
		high_color = LTYELLOW;
	}
	else 
	{
		high_color = LTGREY;
		back_color = bg_color;
	}

	draw();
	set_cursor(IBEAM_CURSOR);
	return 0;
}

void BC_TextBox::set_precision(int precision)
{
	this->precision = precision;
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
	ibeam_letter = text_len;
	draw();
	return 0;
}

int BC_TextBox::update(long value)
{
	char string[BCTEXTLEN];
	sprintf(string, "%ld", value);


	update(string);
	return 0;
}

int BC_TextBox::update(float value)
{
	char string[BCTEXTLEN];
	sprintf(string, "%f", value);
	char *ptr = strchr(string, '.');

//printf("BC_TextBox::update 1 %d\n", precision);
	if(ptr) ptr[precision + 1] = 0;

	update(string);
	return 0;
}

void BC_TextBox::disable()
{
	if(enabled)
	{
		enabled = 0;
		if(active) top_level->deactivate();
		draw();
	}
}

void BC_TextBox::enable()
{
	if(!enabled)
	{
		enabled = 1;
		draw();
	}
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
	if(rows != -1)
	{
		new_h = get_row_h(rows);
		this->rows = rows;
	}
	BC_WindowBase::reposition_window(x, y, w, new_h);
	draw();
	return 0;
}

void BC_TextBox::draw_border()
{
// Clear margins
	set_color(background_color);
	draw_box(0, 0, left_margin, get_h());
	draw_box(get_w() - right_margin, 0, right_margin, get_h());

	if(has_border)
	{
		if(highlighted)
			draw_3d_border(0, 0, w, h,
				top_level->get_resources()->button_shadow, 
				RED, 
				LTPINK,
				top_level->get_resources()->button_light);
		else
			draw_3d_border(0, 0, w, h, 
				top_level->get_resources()->button_shadow, 
				BLACK, 
				top_level->get_resources()->button_up,
				top_level->get_resources()->button_light);
	}
}

void BC_TextBox::draw_cursor()
{
	set_color(background_color);
	set_inverse();
	draw_box(ibeam_x, ibeam_y, BCCURSORW, text_height);
	set_opaque();
}


void BC_TextBox::draw()
{
	int i, j, k, text_len;
	int row_begin, row_end;
	int highlight_x1, highlight_x2;
	int need_ibeam = 1;

// Background
	if(has_border)
	{
		background_color = WHITE;
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
				if(active && enabled)
					set_color(top_level->get_resources()->text_highlight);
				else
					set_color(MEGREY);

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
				set_color(BLACK);
			else
				set_color(MEGREY);

			draw_text(text_x, k + text_ascent, text_row);
// Get ibeam location
			if(ibeam_letter >= row_begin && ibeam_letter <= row_end)
			{
//printf("BC_TextBox::draw 2 %d %d %d\n", row_begin, row_end, ibeam_letter);
				need_ibeam = 0;
				ibeam_y = k;
				ibeam_x = text_x + get_text_width(font, text_row, ibeam_letter - row_begin);
			}
		}
	}

//printf("BC_TextBox::draw 3 %d\n", ibeam_y);
	if(need_ibeam)
	{
		ibeam_x = text_x;
		ibeam_y = text_y;
	}

//printf("BC_TextBox::draw 4 %d\n", ibeam_y);
// Draw solid cursor
	if(!active)
	{
		draw_cursor();
	}

// Border
	draw_border();
	flash();
}

int BC_TextBox::cursor_enter_event()
{
	if(top_level->event_win == win)
	{
		if(!highlighted)
		{
			highlighted = 1;
			draw_border();
			flash();
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
		flash();
	}
	return 0;
}

int BC_TextBox::button_press_event()
{
	if(get_buttonpress() != 1) return 0;

	int cursor_letter = 0;
	int text_len = strlen(text);

	if(!enabled) return 0;

	if(top_level->event_win == win)
	{
		if(!active)
		{
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

int BC_TextBox::repeat_event(long duration)
{
	if(duration == top_level->get_resources()->blink_rate && 
		active)
	{
		draw_cursor();
		flash();
		return 1;
	}
	return 0;
}

void BC_TextBox::default_keypress(int &dispatch_event, int &result)
{
    if((top_level->get_keypress() == RETURN) ||
		(top_level->get_keypress() > 30 && top_level->get_keypress() < 127))
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
	switch(top_level->get_keypress())
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
			top_level->cycle_textboxes(1);
			result = 1;
			break;

		case LEFTTAB:
			top_level->cycle_textboxes(-1);
			result = 1;
			break;

		case LEFT:
			if(ibeam_letter > 0)
			{
// Extend selection
				if(top_level->shift_down())
				{
// Initialize highlighting
					if(highlight_letter1 == highlight_letter2)
					{
						highlight_letter1 = ibeam_letter - 1;
						highlight_letter2 = ibeam_letter;
					}
					else
// Extend left highlight
					if(highlight_letter1 == ibeam_letter)
					{
						highlight_letter1--;
					}
					else
// Shrink right highlight
					if(highlight_letter2 == ibeam_letter)
					{
						highlight_letter2--;
					}
				}
				else
					highlight_letter1 = highlight_letter2;

				ibeam_letter--;

				find_ibeam(1);
				draw();
			}
			result = 1;
			break;

		case RIGHT:
			if(ibeam_letter < text_len)
			{
// Extend selection
				if(top_level->shift_down())
				{
// Initialize highlighting
					if(highlight_letter1 == highlight_letter2)
					{
						highlight_letter1 = ibeam_letter;
						highlight_letter2 = ibeam_letter + 1;
					}
					else
// Shrink left highlight
					if(highlight_letter1 == ibeam_letter)
					{
						highlight_letter1++;
					}
					else
// Expand right highlight
					if(highlight_letter2 == ibeam_letter)
					{
						highlight_letter2++;
					}
				}
				else
					highlight_letter1 = highlight_letter2;

				ibeam_letter++;

				find_ibeam(1);
				draw();
			}
			result = 1;
			break;

		case UP:
			if(ibeam_letter > 0)
			{
// Extend selection
				int new_letter = get_cursor_letter(ibeam_x, ibeam_y - text_height);
//printf("BC_TextBox::keypress_event %d\n", new_letter);

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
				draw();
			}
			result = 1;
			break;

		case DOWN:
//			if(ibeam_letter > 0)
			{
// Extend selection
				int new_letter = get_cursor_letter(ibeam_x, ibeam_y + text_height);
//printf("BC_TextBox::keypress_event %d\n", new_letter);

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
				draw();
			}
			result = 1;
			break;
		
		case END:
			if(top_level->shift_down())
			{
				if(highlight_letter1 == highlight_letter2)
				{
					highlight_letter2 = text_len;
					highlight_letter1 = ibeam_letter;
				}
				else
				if(highlight_letter1 == ibeam_letter)
				{
					highlight_letter1 = highlight_letter2;
					highlight_letter2 = text_len;
				}
				else
				if(highlight_letter2 == ibeam_letter)
				{
					highlight_letter2 = text_len;
				}
			}
			else
				highlight_letter1 = highlight_letter2;

			ibeam_letter = text_len;
			find_ibeam(1);
			draw();
			result = 1;
			break;
		
		case HOME:
			if(top_level->shift_down())
			{
				if(highlight_letter1 == highlight_letter2)
				{
					highlight_letter2 = ibeam_letter;
					highlight_letter1 = 0;
				}
				else
				if(highlight_letter1 == ibeam_letter)
				{
					highlight_letter1 = 0;
				}
				else
				if(highlight_letter2 == ibeam_letter)
				{
					highlight_letter2 = highlight_letter1;
					highlight_letter1 = 0;
				}
			}
			else
				highlight_letter1 = highlight_letter2;

			ibeam_letter = 0;
			find_ibeam(1);
			draw();
			result = 1;
			break;

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
			draw();
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
			draw();
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
					draw();
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
					draw();
					dispatch_event = 1;
					result = 1;
				}
				
				break;
			}

			default_keypress(dispatch_event, result);
			break;
	}

	if(dispatch_event) handle_event();
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
}

void BC_TextBox::get_ibeam_position(int &x, int &y)
{
	int i, j, k, row_begin, row_end, text_len;

	text_len = strlen(text);
	for(i = 0, k = 0; i < text_len; k += text_height)
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
			y = k;
			return;
		}
		if(text[i] == '\n') i++;
	}

	x = 0;
	y = 0;
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

	if((y >= get_h() - text_height - bottom_margin) || 
		(y < top_margin))
	{
		text_y = -(y - (get_h() / 2)) + top_margin;
		if(text_y > top_margin) text_y = top_margin;
	}

	if(dispatch_event && (old_x != text_x || old_y != text_y)) motion_event();
}

int BC_TextBox::get_cursor_letter(int cursor_x, int cursor_y)
{
	int i, j, k, l, row_begin, row_end, text_len, result = 0, done = 0;
	text_len = strlen(text);

	if(cursor_y < 0) 
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
	popup->textbox->update(get_selection(0, 0)->get_text());
	popup->handle_event();
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
	long default_value,
	long min,
	long max,
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
//printf("BC_TumbleTextBoxText::BC_TumbleTextBoxText %f\n", default_value);
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

BC_TumbleTextBox::BC_TumbleTextBox(BC_WindowBase *parent_window, 
		long default_value,
		long min,
		long max,
		int x, 
		int y, 
		int text_w)
{
	this->x = x;
	this->y = y;
	this->min = min;
	this->max = max;
	this->default_value = default_value;
	this->text_w = text_w;
	this->parent_window = parent_window;
	use_float = 0;
	precision = 4;
}

BC_TumbleTextBox::BC_TumbleTextBox(BC_WindowBase *parent_window, 
		float default_value_f,
		float min_f,
		float max_f,
		int x, 
		int y, 
		int text_w)
{
	this->x = x;
	this->y = y;
	this->min_f = min_f;
	this->max_f = max_f;
	this->default_value_f = default_value_f;
	this->text_w = text_w;
	this->parent_window = parent_window;
	use_float = 1;
	precision = 4;
}

BC_TumbleTextBox::~BC_TumbleTextBox()
{
// Recursive delete.  Normally ~BC_TumbleTextBox is never called but textbox
// is deleted anyway by the windowbase so textbox deletes this.
	delete tumbler;
// Don't delete text here if we were called by ~BC_TumbleTextBoxText
	if(textbox)
	{
		textbox->popup = 0;
		delete textbox;
	}
}

void BC_TumbleTextBox::set_precision(int precision)
{
	this->precision = precision;
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

int BC_TumbleTextBox::update(long value)
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


void BC_TumbleTextBox::set_boundaries(long min, long max)
{
	tumbler->set_boundaries(min, max);
}

void BC_TumbleTextBox::set_boundaries(float min, float max)
{
	tumbler->set_boundaries(min, max);
}
