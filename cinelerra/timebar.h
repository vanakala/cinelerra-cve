
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

#ifndef TIMEBAR_H
#define TIMEBAR_H

#include "edl.inc"
#include "guicast.h"
#include "filexml.inc"
#include "labels.inc"
#include "labeledit.inc"
#include "mwindow.inc"
#include "recordlabel.inc"
#include "timebar.inc"
#include "vwindowgui.inc"

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
	LabelGUI(MWindow *mwindow, 
		TimeBar *timebar, 
		int64_t pixel, 
		int y, 
		double position,
		VFrame **data = 0);
	virtual ~LabelGUI();

	static int translate_pixel(MWindow *mwindow, int pixel);
	virtual int handle_event();
	static int get_y(MWindow *mwindow, TimeBar *timebar);
	void reposition();

	Label *label;
	int button_press_event();
	MWindow *mwindow;
	VWindowGUI *gui;
	TimeBar *timebar;
	int64_t pixel;
	double position;
};

class TestPointGUI : public LabelGUI
{
public:
	TestPointGUI(MWindow *mwindow, 
		TimeBar *timebar, 
		int64_t pixel, 
		double position);
};

class InPointGUI : public LabelGUI
{
public:
	InPointGUI(MWindow *mwindow, 
		TimeBar *timebar, 
		int64_t pixel, 
		double position);
	virtual ~InPointGUI();
	static int get_y(MWindow *mwindow, TimeBar *timebar);
};

class OutPointGUI : public LabelGUI
{
public:
	OutPointGUI(MWindow *mwindow, 
		TimeBar *timebar, 
		int64_t pixel, 
		double position);
	virtual ~OutPointGUI();
	static int get_y(MWindow *mwindow, TimeBar *timebar);
};

class TimeBar : public BC_SubWindow
{
public:
	TimeBar(MWindow *mwindow, 
		BC_WindowBase *gui,
		int x, 
		int y,
		int w,
		int h);
	virtual ~TimeBar();

	int create_objects();
	int update_defaults();
	int button_press_event();
	int button_release_event();
	int cursor_motion_event();
	void repeat_event(int64_t duration);

	LabelEdit *label_edit;

// Synchronize label, in/out display with master EDL
	void update(int do_range = 1, int do_others = 1);
	virtual void draw_time();
// Called by update and draw_time.
	virtual void draw_range();
	virtual void select_label(double position);
	virtual EDL* get_edl();
	virtual int test_preview(int buttonpress);
	virtual void update_preview() {};
	virtual int64_t position_to_pixel(double position);
	int move_preview(int &redraw);


	void update_labels();
	void update_points();
// Make sure widgets are highlighted according to selection status
	void update_highlights();
// Update highlight cursor during a drag
	void update_cursor();

// ================================= file operations
	int load(FileXML *xml, int undo_type);
	int draw();                  // draw everything over
	int refresh_labels();

// ========================================= editing

	int select_region(double position);
	void get_edl_length();

	MWindow *mwindow;
	BC_WindowBase *gui;
	int flip_vertical(int w, int h);
// Operation started by a buttonpress
	int current_operation;


private:
	int get_preview_pixels(int &x1, int &x2);
	int draw_bevel();
	ArrayList<LabelGUI*> labels;
	InPointGUI *in_point;
	OutPointGUI *out_point;

// Records for dragging operations
	double start_position;
	double starting_start_position;
	double starting_end_position;
	double time_per_pixel;
	double edl_length;
	int start_cursor_x;
};

#endif
