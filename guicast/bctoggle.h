#ifndef BCTOGGLE_H
#define BCTOGGLE_H

#include "bcbitmap.inc"
#include "bcsubwindow.h"
#include "colors.h"
#include "fonts.h"
#include "vframe.inc"

#define TOGGLE_UP 0
#define TOGGLE_UPHI 1
#define TOGGLE_CHECKED 2
#define TOGGLE_DOWN 3
#define TOGGLE_CHECKEDHI 4

class BC_Toggle : public BC_SubWindow
{
public:
	BC_Toggle(int x, int y, 
		VFrame **data,
		int value, 
		char *caption = "",
		int bottom_justify = 0,
		int font = MEDIUMFONT,
		int color = BLACK);
	virtual ~BC_Toggle();

	virtual int handle_event() { return 0; };
	int get_value();
	int set_value(int value, int draw = 1);
	void set_select_drag(int value);
	int update(int value, int draw = 1);
	void reposition_window(int x, int y);

	int initialize();
	int set_images(VFrame **data);
	int cursor_enter_event();
	int cursor_leave_event();
// In select drag mode these 3 need to be overridden and called back to.
	virtual int button_press_event();
	virtual int button_release_event();
	int cursor_motion_event();
	int repeat_event(int64_t repeat_id);
	int draw_face();

private:
	int has_caption();

	BC_Pixmap *images[5];
	VFrame **data;
	char *caption;
	int status;
	int value;
	int toggle_x, toggle_y, text_x, text_y, text_w, text_h, text_line;
	int bottom_justify;
	int font;
	int color;
	int select_drag;
};

class BC_Radial : public BC_Toggle
{
public:
	BC_Radial(int x, 
		int y, 
		int value, 
		char *caption = "", 
		int font = MEDIUMFONT,
		int color = BLACK);
};

class BC_CheckBox : public BC_Toggle
{
public:
	BC_CheckBox(int x, 
		int y, 
		int value, 
		char *caption = "", 
		int font = MEDIUMFONT,
		int color = BLACK);
	BC_CheckBox(int x, 
		int y, 
		int *value, 
		char *caption = "", 
		int font = MEDIUMFONT,
		int color = BLACK);
	virtual int handle_event();
	int *value;
};

class BC_Label : public BC_Toggle
{
public:
	BC_Label(int x, 
		int y, 
		int value, 
		int font = MEDIUMFONT,
		int color = BLACK);
};

#endif
