// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef BCTEXTBOX_H
#define BCTEXTBOX_H

#include "bclistbox.h"
#include "bcsubwindow.h"
#include "bctumble.h"
#include "fonts.h"
#include "bctimer.inc"

#define BCCURSORW 2
#define TEXTBOXLEN (BCTEXTLEN - 1)

class BC_TextBox : public BC_SubWindow
{
public:
	BC_TextBox(int x, int y, int w, int rows, const char *text,
		int options = TXTBOX_BORDER, int font = MEDIUMFONT,
		int is_utf8 = 0);
	BC_TextBox(int x, int y, int w, int rows,
		const char *text, const wchar_t *wtext,
		int options = TXTBOX_BORDER,
		int font = MEDIUMFONT);
	BC_TextBox(int x, int y, int w, int rows, int text,
		int options = TXTBOX_BORDER,
		int font = MEDIUMFONT);
	BC_TextBox(int x, int y, int w, int rows, int64_t text,
		int options = TXTBOX_BORDER,
		int font = MEDIUMFONT);
	BC_TextBox(int x, int y, int w, int rows, double text,
		int options = TXTBOX_BORDER,
		int font = MEDIUMFONT,
		int precision = 4);
	virtual ~BC_TextBox();

// Whenever the contents of the text change
	virtual int handle_event() { return 0; };
// Whenever the position of the text changes
	virtual void motion_event() {};
	void set_selection(int char1, int char2, int ibeam);
	void update(const char *text);
	void update(const wchar_t *text);
	void updateutf8(const char *text);
	void update(int value);
	void update(double value);
	static wchar_t *wstringbreaker(int font, wchar_t *text,
		int boxwidth, BC_WindowBase *win);

// options: do not dim the text when the box is disabled
	void disable(int options = 0);
	void enable();
	inline int get_enabled() { return enabled; };

	void initialize();

	void focus_in_event();
	void focus_out_event();
	int cursor_enter_event();
	void cursor_leave_event();
	int cursor_motion_event();
	virtual int button_press_event();
	int button_release_event();
	void repeat_event(int repeat_id);
	int keypress_event();
	void activate();
	void deactivate();
	char* get_text();
	char* get_utf8text();
	wchar_t* get_wtext(int *length = 0);
	int get_text_rows();
// Set top left of text view
	void set_text_row(int row);
	int get_text_row();
	void reposition_window(int x, int y, int w = -1, int rows = -1);
	static int calculate_h(BC_WindowBase *gui, int font, int has_border, int rows);
	static int calculate_row_h(int rows, BC_WindowBase *parent_window,
		int has_border = 1, int font = MEDIUMFONT);
	static int pixels_to_rows(BC_WindowBase *window, int font, int pixels);
	void set_precision(int precision);
// Whether to draw every time there is a keypress or rely on user to
// follow up every keypress with an update().
	void set_keypress_draw(int value);
	inline int get_ibeam_letter() { return ibeam_letter; };
	void set_ibeam_letter(int number, int redraw = 1);
// Used for custom formatting text boxes
	inline int get_last_keypress() { return last_keypress; };
// Table of separators to skip.  Used by time textboxes
// The separator format is "0000:0000".  Things not alnum are considered
// separators.  The alnums are replaced by user text.
	void set_separators(const char *separators);

// 1 - selects text, -1 - deselects, 0 - do nothing
// in all cases it returns text_selected after the operation
	int select_whole_text(int select);
	void cycle_textboxes(int amout);

protected:
	int defaultcolor;

private:
	void convert_number();
	void reset_parameters(int rows, int has_border, int font);
	void draw();
	void draw_border();
	void draw_cursor();
	void copy_selection(int clipboard_num);
	void paste_selection(int clipboard_num);
	void delete_selection(int letter1, int letter2, int text_len);
	void insert_text(const wchar_t *string, int string_len = -1);
	void update_wtext();
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
	int break_string;
	int font;
	int rows;
	int highlighted;
	int high_color, back_color;
	int background_color;
	char ntext[TEXTBOXLEN + 1];
	int wtext_len;
	int *positions;
	int active;
	int enabled;
	int precision;
	int keypress_draw;
// Cause the repeater to skip a cursor refresh if a certain event happened
// within a certain time of the last repeat event
	Timer *skip_cursor;
// Used for custom formatting text boxes
	int last_keypress;
	const char *separators;
};


class BC_ScrollTextBoxText;
class BC_ScrollTextBoxYScroll;


class BC_ScrollTextBox
{
public:
	BC_ScrollTextBox(BC_WindowBase *parent_window,
		int x, int y, int w, int rows,
		const char *default_text);
	BC_ScrollTextBox(BC_WindowBase *parent_window,
		int x, int y, int w, int rows,
		const wchar_t *default_text);
	virtual ~BC_ScrollTextBox();

	virtual int handle_event() { return 0; };
	char* get_text();
	wchar_t* get_wtext(int *length = 0);
	void update(const char *text);
	void update(const wchar_t *text);
	void reposition_window(int x, int y, int w, int rows);
	inline int get_x() { return x; };
	int get_y() { return y; };
	int get_w() { return w; };
// Visible rows for resizing
	int get_rows() { return rows; };

	friend class BC_ScrollTextBoxText;
	friend class BC_ScrollTextBoxYScroll;

private:
	BC_ScrollTextBoxText *text;
	BC_ScrollTextBoxYScroll *yscroll;
	BC_WindowBase *parent_window;
	const char *default_text;
	const wchar_t *default_wtext;
	int x, y, w, rows;
};

class BC_ScrollTextBoxText : public BC_TextBox
{
public:
	BC_ScrollTextBoxText(BC_ScrollTextBox *gui);
	virtual ~BC_ScrollTextBoxText();

	int handle_event();
	void motion_event();

	BC_ScrollTextBox *gui;
};

class BC_ScrollTextBoxYScroll : public BC_ScrollBar
{
public:
	BC_ScrollTextBoxYScroll(BC_ScrollTextBox *gui);
	virtual ~BC_ScrollTextBoxYScroll() {};

	int handle_event();
	BC_ScrollTextBox *gui;
};


class BC_PopupTextBoxText;
class BC_PopupTextBoxList;

class BC_PopupTextBox
{
public:
	BC_PopupTextBox(BC_WindowBase *parent_window, 
		ArrayList<BC_ListBoxItem*> *list_items,
		const char *default_text,
		int x, int y, int text_w, int list_h);
	virtual ~BC_PopupTextBox();

	virtual int handle_event() { return 0; };
	char* get_text();
	int get_number();
	int get_x() { return x; };
	int get_y() { return y; };
	int get_w();
	int get_h();
	void update(const char *text);
	void update_list(ArrayList<BC_ListBoxItem*> *data);
	void reposition_widget(int x, int y, int w = -1, int h = -1);
	void reposition_window(int x, int y);
	void disable_text(int option);

	friend class BC_PopupTextBoxText;
	friend class BC_PopupTextBoxList;

private:
	int x, y, text_w, list_h;
	const char *default_text;
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

class BC_TumbleTextBox
{
public:
	BC_TumbleTextBox(BC_WindowBase *parent_window, 
		int default_value, int min, int max,
		int x, int y, int text_w);
	BC_TumbleTextBox(BC_WindowBase *parent_window,
		double default_value, double min, double max,
		int x, int y, int text_w, int precision = 4);
	virtual ~BC_TumbleTextBox();

	virtual int handle_event() { return 0; };
	char* get_text();
	void update(const char *value);
	void update(int value);
	void update(double value);
	int get_x() { return x; };
	int get_y() { return y; };
	int get_w();
	int get_h();
	void reposition_widget(int x, int y, int w = -1, int h = -1);
	void reposition_window(int x, int y);
	void set_boundaries(int min, int max);
	void set_boundaries(float min, float max);
	void set_boundaries(double min, double max);
	void set_precision(int precision);
	void set_increment(double value);
	void set_log_floatincrement(int value);

	friend class BC_TumbleTextBoxText;
	friend class BC_TumbleTextBoxTumble;

private:
	void reset();

	int x, y, text_w;
	int default_value, min, max;
	double default_value_f, min_f, max_f;
	int use_float;
	int precision;
	double increment;
	int log_floatincrement;
	BC_TumbleTextBoxText *textbox;
	BC_Tumbler *tumbler;
	BC_WindowBase *parent_window;
};

class BC_TumbleTextBoxText : public BC_TextBox
{
public:
	BC_TumbleTextBoxText(BC_TumbleTextBox *popup,
		int default_value, int min, int max,
		int x, int y);
	BC_TumbleTextBoxText(BC_TumbleTextBox *popup,
		double default_value, double min, double max,
		int x, int y, int precision);
	BC_TumbleTextBoxText(BC_TumbleTextBox *popup, int x, int y);
	virtual ~BC_TumbleTextBoxText();

	int handle_event();
	int button_press_event();
	BC_TumbleTextBox *popup;
};
#endif
