#ifndef BCBUTTON_H
#define BCBUTTON_H

#include "bcbitmap.inc"
#include "bcsubwindow.h"
#include "vframe.inc"

#include <stdint.h>


class BC_Button : public BC_SubWindow
{
public:
	BC_Button(int x, int y, VFrame **data);
	BC_Button(int x, int y, int w, VFrame **data);
	virtual ~BC_Button();

	friend class BC_GenericButton;

	virtual int handle_event() { return 0; };
	int repeat_event(int64_t repeat_id);
	virtual int draw_face();

	int initialize();
	virtual int set_images(VFrame **data);
	int cursor_enter_event();
	int cursor_leave_event();
	int button_press_event();
	int button_release_event();
	int cursor_motion_event();
	int update_bitmaps(VFrame **data);
	int reposition_window(int x, int y);
	void set_underline(int number);

private:

	BC_Pixmap *images[3];
	VFrame **data;
	int status;
	int w_argument;
	int underline_number;
};




class BC_GenericButton : public BC_Button
{
public:
	BC_GenericButton(int x, int y, char *text, VFrame **data = 0);
	BC_GenericButton(int x, int y, int w, char *text, VFrame **data = 0);
	int set_images(VFrame **data);
	int draw_face();

private:
	char text[BCTEXTLEN];
};

class BC_OKButton : public BC_Button
{
public:
	BC_OKButton(int x, int y);
	BC_OKButton(BC_WindowBase *parent_window);
	virtual int resize_event(int w, int h);
	virtual int handle_event();
	virtual int keypress_event();
};

class BC_CancelButton : public BC_Button
{
public:
	BC_CancelButton(int x, int y);
	BC_CancelButton(BC_WindowBase *parent_window);
	virtual int resize_event(int w, int h);
	virtual int handle_event();
	virtual int keypress_event();
};

#endif
