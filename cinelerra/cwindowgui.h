// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef CWINDOWGUI_H
#define CWINDOWGUI_H

#include "auto.inc"
#include "bcslider.h"
#include "bcwindow.h"
#include "canvas.h"
#include "cpanel.inc"
#include "ctimebar.inc"
#include "cwindow.inc"
#include "cwindowtool.inc"
#include "editpanel.h"
#include "floatauto.inc"
#include "floatautos.inc"
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


class CWindowGUI : public BC_Window
{
public:
	CWindowGUI(MWindow *mwindow, CWindow *cwindow);
	~CWindowGUI();

	void resize_event(int w, int h);

// Events for the fullscreen canvas fall through to here.
	int button_press_event();
	void cursor_leave_event();
	int cursor_enter_event();
	int button_release_event();
	int cursor_motion_event();

	void zoom_canvas(int do_auto, double value, int update_menu);

	void close_event();
	int keypress_event();
	void translation_event();
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
	CPanel *composite_panel;
	CWindowZoom *zoom_panel;
	CWindowSlider *slider;
	CWindowReset *reset;
	CWindowTransport *transport;
	CWindowCanvas *canvas;
	CTimeBar *timebar;
	BC_Pixmap *active;
	BC_Pixmap *inactive;

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
	double x_offset, y_offset;
// Cursor location during the last button press relative to output
// and offset by scroll bars
	double x_origin, y_origin;
// Crop handle being dragged
	int crop_handle;
// If dragging crop translation
	int crop_translate;
// Origin of crop handle during last button press
	double crop_origin_x, crop_origin_y;
// Origin of all 4 crop points during last button press
	double crop_origin_x1, crop_origin_y1;
	double crop_origin_x2, crop_origin_y2;

	double ruler_origin_x, ruler_origin_y;
	int ruler_handle;
	int ruler_translate;

// Origin for camera and projector operations during last button press
	double center_x, center_y, center_z;
	double control_in_x, control_in_y, control_out_x, control_out_y;
	int current_tool;
// Must recalculate the origin when pressing shift.
// Switch toggle on and off to recalculate origin.
	int translating_zoom;
};


class CWindowEditing : public EditPanel
{
public:
	CWindowEditing(MWindow *mwindow, CWindowGUI *gui, MeterPanel *meter_panel);

	void set_inpoint();
	void set_outpoint();

	MWindow *mwindow;
};


class CWindowMeters : public MeterPanel
{
public:
	CWindowMeters(MWindow *mwindow, CWindowGUI *gui, int x, int y, int h);

	int change_status_event();

	MWindow *mwindow;
	CWindowGUI *gui;
};


class CWindowZoom : public ZoomPanel
{
public:
	CWindowZoom(MWindow *mwindow, CWindowGUI *gui, int x, int y,
		const char *first_item_text);

	int handle_event();
	MWindow *mwindow;
	CWindowGUI *gui;
};


class CWindowSlider : public BC_PercentageSlider
{
public:
	CWindowSlider(MWindow *mwindow, CWindow *cwindow, int x, int y, int pixels);

	int handle_event();
	void set_position();
	void increase_value();
	void decrease_value();

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


class CWindowTransport : public PlayTransport
{
public:
	CWindowTransport(MWindow *mwindow, CWindowGUI *gui, 
		int x, int y);

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
	void zoom_resize_window(double percentage);
	void update_zoom(int x, int y, double zoom);
	int get_xscroll();
	int get_yscroll();
	double get_zoom();
	int do_eyedrop(int &rerender, int button_press);
	int do_mask(int &redraw,
		int &rerender,
		int button_press,
		int cursor_motion,
		int draw);
	void draw_refresh();
	void draw_overlays();
	void update_guidelines();
// Cursor may have to be drawn
	int cursor_leave_event();
	int cursor_enter_event();
	int cursor_motion_event();
	int button_press_event();
	int button_release_event();
	int get_fullscreen();
	void set_fullscreen(int value);
	int test_crop(int button_press, int *redraw, int *rerender = 0);
	int test_bezier(int button_press, 
		int &redraw,
		int &redraw_canvas,
		int &rerender,
		int do_camera);
	int do_ruler(int draw, int motion, int button_press, int button_release);
	void test_zoom(int &redraw);
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
	double sample_aspect_ratio();

	MWindow *mwindow;
	CWindowGUI *gui;
	GuideFrame *safe_regions;
};

#endif
