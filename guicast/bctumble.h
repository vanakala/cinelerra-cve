// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef BCTUMBLE_H
#define BCTUMBLE_H

#include "bcsubwindow.h"

class BC_Tumbler : public BC_SubWindow
{
public:
	BC_Tumbler(int x, int y, VFrame **data = 0);
	virtual ~BC_Tumbler();

	virtual void handle_up_event() {};
	virtual void handle_down_event() {};
	void repeat_event(int repeat_id);

	void initialize();
	void set_images(VFrame **data);
	int cursor_enter_event();
	void cursor_leave_event();
	int button_press_event();
	int button_release_event();
	int cursor_motion_event();
	void update_bitmaps(VFrame **data);
	void reposition_window(int x, int y, int w=-1, int h=-1); // w & h don't do anything, except to inherit BC_Subwindow::(reposition_window)
	virtual void set_boundaries(int min, int max) {};
	virtual void set_boundaries(double min, double max) {};
	virtual void set_increment(int value) {};
	virtual void set_increment(double value) {};
	virtual void set_log_floatincrement(int value) {};

private:
	void draw_face();

	BC_Pixmap *images[4];
	int status;
	int repeat_count;
	VFrame **data;
};

class BC_ITumbler : public BC_Tumbler
{
public:
	BC_ITumbler(BC_TextBox *textbox, int min, int max, int x, int y);
	virtual ~BC_ITumbler() {};

	void handle_up_event();
	void handle_down_event();
	void set_increment(int value);
	void set_boundaries(int min, int max);

	int min, max;
	int increment;
	BC_TextBox *textbox;
};

class BC_FTumbler : public BC_Tumbler
{
public:
	BC_FTumbler(BC_TextBox *textbox, double min, double max, int x, int y);
	virtual ~BC_FTumbler() {};

	void handle_up_event();
	void handle_down_event();
	void set_boundaries(double min, double max);
	void set_increment(double value);
	void set_log_floatincrement(int value);

	double min, max;
	double increment;
	int log_floatincrement;
	BC_TextBox *textbox;
};

#endif
