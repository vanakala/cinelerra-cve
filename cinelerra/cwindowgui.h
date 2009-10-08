
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

#ifndef CWINDOWGUI_H
#define CWINDOWGUI_H

#include "auto.inc"
#include "canvas.h"
#include "cpanel.inc"
#include "ctimebar.inc"
#include "cwindow.inc"
#include "cwindowtool.inc"
#include "editpanel.h"
#include "floatauto.inc"
#include "floatautos.inc"
#include "guicast.h"
#include "mainclock.inc"
#include "maskauto.inc"
#include "meterpanel.h"
#include "mwindow.inc"
#include "playtransport.h"
#include "thread.h"
#include "track.inc"
#include "zoompanel.h"

class CWindowZoom;
class CWindowSlider;
class CWindowReset;
class CWindowDestination;
class CWindowMeters;
class CWindowTransport;
class CWindowCanvas;
class CWindowEditing;


#define AUTO_ZOOM "Auto"

class CWindowGUI : public BC_Window
{
public:
	CWindowGUI(MWindow *mwindow, CWindow *cwindow);
	~CWindowGUI();

    int create_objects();
	int resize_event(int w, int h);

// Events for the fullscreen canvas fall through to here.
	int button_press_event();
	int cursor_leave_event();
	int cursor_enter_event();
	int button_release_event();
	int cursor_motion_event();


	void zoom_canvas(int do_auto, double value, int update_menu);

	int close_event();
	int keypress_event();
	int translation_event();
	void set_operation(int value);
	void update_tool();
	void drag_motion();
	int drag_stop();
	void draw_status();
// Zero out pointers to affected auto
	void reset_affected();
	void keyboard_zoomin();
	void keyboard_zoomout();

	MWindow *mwindow;
    CWindow *cwindow;
	CWindowEditing *edit_panel;
//	APanel *automation_panel;
	CPanel *composite_panel;
	CWindowZoom *zoom_panel;
	CWindowSlider *slider;
	CWindowReset *reset;
	CWindowTransport *transport;
	CWindowCanvas *canvas;
	CTimeBar *timebar;
	BC_Pixmap *active;
	BC_Pixmap *inactive;
//	MainClock *clock;


	CWindowMeters *meters;


	CWindowTool *tool_panel;

// Cursor motion modification being done
	int current_operation;
// The pointers are only used for dragging.  Information for tool windows
// must be integers and recalculted for every keypress.
// Track being modified in canvas
	Track *affected_track;
// Transformation keyframe being modified
	FloatAuto *affected_x;
	FloatAuto *affected_y;
	FloatAuto *affected_z;
// Keyfrom not affecting transformation being affected
	Auto *affected_keyframe;
// Mask point being modified
	int affected_point;
// Scrollbar offsets during last button press relative to output
	float x_offset, y_offset;
// Cursor location during the last button press relative to output
// and offset by scroll bars
	float x_origin, y_origin;
// Crop handle being dragged
	int crop_handle;
// If dragging crop translation
	int crop_translate;
// Origin of crop handle during last button press
	float crop_origin_x, crop_origin_y;
// Origin of all 4 crop points during last button press
	float crop_origin_x1, crop_origin_y1;
	float crop_origin_x2, crop_origin_y2;

// Origin for camera and projector operations during last button press
	float center_x, center_y, center_z;
	float control_in_x, control_in_y, control_out_x, control_out_y;
	int current_tool;
// Must recalculate the origin when pressing shift.
// Switch toggle on and off to recalculate origin.
	int translating_zoom;
};


class CWindowEditing : public EditPanel
{
public:
	CWindowEditing(MWindow *mwindow, CWindow *cwindow);
	
	void set_inpoint();
	void set_outpoint();

	MWindow *mwindow;
	CWindow *cwindow;
};


class CWindowMeters : public MeterPanel
{
public:
	CWindowMeters(MWindow *mwindow, CWindowGUI *gui, int x, int y, int h);
	~CWindowMeters();
	
	int change_status_event();
	
	MWindow *mwindow;
	CWindowGUI *gui;
};

class CWindowZoom : public ZoomPanel
{
public:
	CWindowZoom(MWindow *mwindow, CWindowGUI *gui, int x, int y);
	~CWindowZoom();
	int handle_event();
	MWindow *mwindow;
	CWindowGUI *gui;
};


class CWindowSlider : public BC_PercentageSlider
{
public:
	CWindowSlider(MWindow *mwindow, CWindow *cwindow, int x, int y, int pixels);
	~CWindowSlider();

	int handle_event();
	void set_position();
	int increase_value();
	int decrease_value();

	MWindow *mwindow;
	CWindow *cwindow;
};

class CWindowReset : public BC_Button
{
public:
	CWindowReset(MWindow *mwindow, CWindowGUI *cwindow, int x, int y);
	~CWindowReset();
	int handle_event();
	CWindowGUI *cwindow;
	MWindow *mwindow;
};

// class CWindowDestination : public BC_PopupTextBox
// {
// public:
// 	CWindowDestination(MWindow *mwindow, CWindowGUI *cwindow, int x, int y);
// 	~CWindowDestination();
// 	int handle_event();
// 	CWindowGUI *cwindow;
// 	MWindow *mwindow;
// };

class CWindowTransport : public PlayTransport
{
public:
	CWindowTransport(MWindow *mwindow, 
		CWindowGUI *gui, 
		int x, 
		int y);
	EDL* get_edl();
	void goto_start();
	void goto_end();

	CWindowGUI *gui;
};


class CWindowCanvas : public Canvas
{
public:
	CWindowCanvas(MWindow *mwindow, CWindowGUI *gui);

	void status_event();
	void zoom_resize_window(float percentage);
	void update_zoom(int x, int y, float zoom);
	int get_xscroll();
	int get_yscroll();
	float get_zoom();
	int do_eyedrop(int &rerender, int button_press);
	int do_mask(int &redraw, 
		int &rerender, 
		int button_press, 
		int cursor_motion,
		int draw);
	void draw_refresh();
	void draw_overlays();
	void draw_safe_regions();
// Cursor may have to be drawn
	int cursor_leave_event();
	int cursor_enter_event();
	int cursor_motion_event();
	int button_press_event();
	int button_release_event();
	int get_fullscreen();
	void set_fullscreen(int value);
	int test_crop(int button_press, int &redraw);
	int test_bezier(int button_press, 
		int &redraw, 
		int &redraw_canvas, 
		int &rerender,
		int do_camera);
	int test_zoom(int &redraw);
	void reset_camera();
	void reset_projector();
	void reset_keyframe(int do_camera);
	void draw_crophandle(int x, int y);
	void zoom_auto();

// Draw the projector overlay in different colors.
	void draw_bezier(int do_camera);
	void draw_crop();
	void calculate_origin();
	void toggle_controls();
	int get_cwindow_controls();

	MWindow *mwindow;
	CWindowGUI *gui;
};

#endif
