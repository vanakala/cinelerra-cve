
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

#ifndef BCTEXTBOX_H
#define BCTEXTBOX_H

#include "bclistbox.h"
#include "bcsubwindow.h"
#include "bctumble.h"
#include "fonts.h"
#include "bctimer.inc"

#define BCCURSORW 2



class BC_TextBox : public BC_SubWindow
{
public:
	BC_TextBox(int x, 
		int y, 
		int w, 
		int rows, 
		char *text, 
		int has_border = 1, 
		int font = MEDIUMFONT);
	BC_TextBox(int x, 
		int y, 
		int w, 
		int rows, 
		int64_t text, 
		int has_border = 1, 
		int font = MEDIUMFONT);
	BC_TextBox(int x, 
		int y, 
		int w, 
		int rows, 
		int text, 
		int has_border = 1, 
		int font = MEDIUMFONT);
	BC_TextBox(int x, 
		int y, 
		int w, 
		int rows, 
		float text, 
		int has_border = 1, 
		int font = MEDIUMFONT,
		int precision = 4);
	virtual ~BC_TextBox();

// Whenever the contents of the text change
	virtual int handle_event() { return 0; };
// Whenever the position of the text changes
	virtual int motion_event() { return 0; };
	void set_selection(int char1, int char2, int ibeam);
	int update(char *text);
	int update(int64_t value);
	int update(float value);
	void disable();
	void enable();
	int get_enabled();

	int initialize();

	int focus_in_event();
	int focus_out_event();
	int cursor_enter_event();
	int cursor_leave_event();
	int cursor_motion_event();
	virtual int button_press_event();
	int button_release_event();
	int repeat_event(int64_t repeat_id);
	int keypress_event();
	int activate();
	int deactivate();
	char* get_text();
	int get_text_rows();
// Set top left of text view
	void set_text_row(int row);
	int get_text_row();
	int reposition_window(int x, int y, int w = -1, int rows = -1);
	int uses_text();
	static int calculate_h(BC_WindowBase *gui, int font, int has_border, int rows);
	static int calculate_row_h(int rows, BC_WindowBase *parent_window, int has_border = 1, int font = MEDIUMFONT);
	static int pixels_to_rows(BC_WindowBase *window, int font, int pixels);
	void set_precision(int precision);
// Whether to draw every time there is a keypress or rely on user to
// follow up every keypress with an update().
	void set_keypress_draw(int value);
	int get_ibeam_letter();
	void set_ibeam_letter(int number, int redraw = 1);
// Used for custom formatting text boxes
	int get_last_keypress();
// Table of separators to skip.  Used by time textboxes
// The separator format is "0000:0000".  Things not alnum are considered
// separators.  The alnums are replaced by user text.
	void set_separators(char *separators);

// 1 - selects text, -1 - deselects, 0 - do nothing
// in all cases it returns text_selected after the operation
	int select_whole_text(int select);
	void cycle_textboxes(int amout);
	
private:
	int reset_parameters(int rows, int has_border, int font);
	void draw();
	void draw_border();
	void draw_cursor();
	void copy_selection(int clipboard_num);
	void paste_selection(int clipboard_num);
	void delete_selection(int letter1, int letter2, int text_len);
	void insert_text(char *string);
// Reformat text according to separators.
// ibeam_left causes the ibeam to move left.
	void do_separators(int ibeam_left);
	void get_ibeam_position(int &x, int &y);
	void find_ibeam(int dispatch_event);
	void select_word(int &letter1, int &letter2, int ibeam_letter);
	int get_cursor_letter(int cursor_x, int cursor_y);
	int get_row_h(int rows);
	void default_keypress(int &dispatch_event, int &result);


// Top left of text relative to window
	int text_x, text_y;
// Top left of cursor relative to text
	int ibeam_x, ibeam_y;

	int ibeam_letter;
	int highlight_letter1, highlight_letter2;
	int highlight_letter3, highlight_letter4;
	int text_x1, text_start, text_end, text_selected, word_selected;
	int text_ascent, text_descent, text_height;
	int left_margin, right_margin, top_margin, bottom_margin;
	int has_border;
	int font;
	int rows;
	int highlighted;
	int high_color, back_color;
	int background_color;
	char text[BCTEXTLEN], text_row[BCTEXTLEN], temp_string[2];
	int active;
	int enabled;
	int precision;
	int keypress_draw;
// Cause the repeater to skip a cursor refresh if a certain event happened
// within a certain time of the last repeat event
	Timer *skip_cursor;
// Used for custom formatting text boxes
	int last_keypress;
	char *separators;
};



class BC_ScrollTextBoxText;
class BC_ScrollTextBoxYScroll;


class BC_ScrollTextBox
{
public:
	BC_ScrollTextBox(BC_WindowBase *parent_window, 
		int x, 
		int y, 
		int w,
		int rows,
		char *default_text);
	virtual ~BC_ScrollTextBox();
	void create_objects();
	virtual int handle_event();
	
	char* get_text();
	void update(char *text);
	void reposition_window(int x, int y, int w, int rows);
	int get_x();
	int get_y();
	int get_w();
// Visible rows for resizing
	int get_rows();

	friend class BC_ScrollTextBoxText;
	friend class BC_ScrollTextBoxYScroll;

private:
	BC_ScrollTextBoxText *text;
	BC_ScrollTextBoxYScroll *yscroll;
	BC_WindowBase *parent_window;
	char *default_text;
	int x, y, w, rows;
};

class BC_ScrollTextBoxText : public BC_TextBox
{
public:
	BC_ScrollTextBoxText(BC_ScrollTextBox *gui);
	virtual ~BC_ScrollTextBoxText();
	int handle_event();
	int motion_event();
	BC_ScrollTextBox *gui;
};

class BC_ScrollTextBoxYScroll : public BC_ScrollBar
{
public:
	BC_ScrollTextBoxYScroll(BC_ScrollTextBox *gui);
	virtual ~BC_ScrollTextBoxYScroll();
	int handle_event();
	BC_ScrollTextBox *gui;
};




class BC_PopupTextBoxText;
class BC_PopupTextBoxList;

class BC_PopupTextBox : public BC_RelocatableWidget
{
public:
	BC_PopupTextBox(BC_WindowBase *parent_window, 
		ArrayList<BC_ListBoxItem*> *list_items,
		char *default_text,
		int x, 
		int y, 
		int text_w,
		int list_h);
	virtual ~BC_PopupTextBox();
	int create_objects();
	virtual int handle_event();
	char* get_text();
	int get_number();
	int get_x();
	int get_y();
	int get_w();
	int get_h();
	void update(char *text);
	void update_list(ArrayList<BC_ListBoxItem*> *data);
	int  reposition_widget(int x, int y, int w = -1, int h = -1);
	void reposition_window(int x, int y);

	friend class BC_PopupTextBoxText;
	friend class BC_PopupTextBoxList;

private:
	int x, y, text_w, list_h;
	char *default_text;
	ArrayList<BC_ListBoxItem*> *list_items;
	BC_PopupTextBoxText *textbox;
	BC_PopupTextBoxList *listbox;
	BC_WindowBase *parent_window;
};

class BC_PopupTextBoxText : public BC_TextBox
{
public:
	BC_PopupTextBoxText(BC_PopupTextBox *popup, int x, int y);
	virtual ~BC_PopupTextBoxText();
	int handle_event();
	BC_PopupTextBox *popup;
};

class BC_PopupTextBoxList : public BC_ListBox
{
public:
	BC_PopupTextBoxList(BC_PopupTextBox *popup, int x, int y);
	int handle_event();
	BC_PopupTextBox *popup;
};


class BC_TumbleTextBoxText;
class BC_TumbleTextBoxTumble;

class BC_TumbleTextBox : public BC_RelocatableWidget
{
public:
	BC_TumbleTextBox(BC_WindowBase *parent_window, 
		int64_t default_value,
		int64_t min,
		int64_t max,
		int x, 
		int y, 
		int text_w);
	BC_TumbleTextBox(BC_WindowBase *parent_window, 
		int default_value,
		int min,
		int max,
		int x, 
		int y, 
		int text_w);
	BC_TumbleTextBox(BC_WindowBase *parent_window, 
		float default_value,
		float min,
		float max,
		int x, 
		int y, 
		int text_w);
	virtual ~BC_TumbleTextBox();

	int create_objects();
	void reset();
	virtual int handle_event();
	char* get_text();
	int update(char *value);
	int update(int64_t value);
	int update(float value);
	int get_x();
	int get_y();
	int get_w();
	int get_h();
	int  reposition_widget(int x, int y, int w = -1, int h = -1);
	void reposition_window(int x, int y);
	void set_boundaries(int64_t min, int64_t max);
	void set_boundaries(float min, float max);
	void set_precision(int precision);
	void set_increment(float value);
	void set_log_floatincrement(int value);

	friend class BC_TumbleTextBoxText;
	friend class BC_TumbleTextBoxTumble;

private:
	int x, y, text_w;
	int64_t default_value, min, max;
	float default_value_f, min_f, max_f;
	int use_float;
	int precision;
	float increment;
	int log_floatincrement;
	BC_TumbleTextBoxText *textbox;
	BC_Tumbler *tumbler;
	BC_WindowBase *parent_window;
};

class BC_TumbleTextBoxText : public BC_TextBox
{
public:
	BC_TumbleTextBoxText(BC_TumbleTextBox *popup, 
		int64_t default_value,
		int64_t min,
		int64_t max,
		int x, 
		int y);
	BC_TumbleTextBoxText(BC_TumbleTextBox *popup, 
		float default_value,
		float min,
		float max,
		int x, 
		int y);
	BC_TumbleTextBoxText(BC_TumbleTextBox *popup, int x, int y);
	virtual ~BC_TumbleTextBoxText();
	int handle_event();
	int button_press_event();
	BC_TumbleTextBox *popup;
};


#endif
