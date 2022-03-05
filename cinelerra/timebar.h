// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef TIMEBAR_H
#define TIMEBAR_H

#include "bctoggle.h"
#include "bcsubwindow.h"
#include "datatype.h"
#include "edl.inc"
#include "labels.inc"
#include "labeledit.inc"
#include "timebar.inc"

class TimeBarLeftArrow;
class TimeBarRightArrow;

class LabelGUI;
class InPointGUI;
class OutPointGUI;

// Operations for cursor
#define TIMEBAR_NONE        0
#define TIMEBAR_DRAG        1
#define TIMEBAR_DRAG_LEFT   2
#define TIMEBAR_DRAG_RIGHT  3
#define TIMEBAR_DRAG_CENTER 4

class LabelGUI : public BC_Toggle
{
public:
	LabelGUI(TimeBar *timebar,
		int pixel,
		int y, 
		ptstime position,
		VFrame **data = 0);

	static int translate_pixel(int pixel);
	virtual int handle_event();
	static int get_y(TimeBar *timebar);
	void reposition();
	int button_press_event();

	Label *label;
	TimeBar *timebar;
	int pixel;
	ptstime position;
};

class InPointGUI : public LabelGUI
{
public:
	InPointGUI(TimeBar *timebar,
		int pixel,
		ptstime position);

	static int get_y(TimeBar *timebar);
};

class OutPointGUI : public LabelGUI
{
public:
	OutPointGUI(TimeBar *timebar,
		int pixel, 
		ptstime position);

	static int get_y(TimeBar *timebar);
};

class TimeBar : public BC_SubWindow
{
public:
	TimeBar(BC_WindowBase *gui,
		int x, 
		int y,
		int w,
		int h);
	virtual ~TimeBar();

	int update_defaults();
	int button_press_event();
	int button_release_event();
	int cursor_motion_event();
	void repeat_event(int duration);

	LabelEdit *label_edit;

// Synchronize label, in/out display with master EDL
	void update(int fast = 0);
	virtual void draw_time() {};
// Called by update and draw_time.
	virtual void draw_range();
	virtual void select_label(ptstime position) {};
	virtual EDL* get_edl();
	virtual int test_preview(int buttonpress);
	virtual void update_preview() {};
	virtual int position_to_pixel(ptstime position);
	int move_preview(int &redraw);

	void update_labels();
	void update_points();
// Update highlight of current widget
	void update_highlight(BC_Toggle *toggle, ptstime position);
// Make sure widgets are highlighted according to selection status
	void update_highlights();
// Update highlight cursor during a drag
	void update_cursor();

// ========================================= editing

	void select_region(ptstime position);
	void get_edl_length();

	BC_WindowBase *gui;
	int flip_vertical(int w, int h);
// Operation started by a buttonpress
	int current_operation;

private:
	void get_preview_pixels(int &x1, int &x2);

	ArrayList<LabelGUI*> labels;
	InPointGUI *in_point;
	OutPointGUI *out_point;

// Records for dragging operations
	ptstime start_position;
	ptstime starting_start_position;
	ptstime starting_end_position;
	ptstime time_per_pixel;
	ptstime edl_length;
	int start_cursor_x;
};

#endif
