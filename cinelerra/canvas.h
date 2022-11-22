// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef CANVAS_H
#define CANVAS_H

#include "bcpopup.h"
#include "bcmenuitem.h"
#include "bcpopupmenu.h"
#include "bcscrollbar.h"
#include "cwindowgui.inc"
#include "edl.inc"
#include "guidelines.h"
#include "vwindowgui.inc"

// Output for all X11 video

class CanvasOutput;
class CanvasFullScreen;
class CanvasXScroll;
class CanvasYScroll;
class CanvasPopup;
class CanvasFullScreenPopup;
class CanvasToggleControls;

// The EDL arguments can be set to 0 if the canvas_w and canvas_h are used
class Canvas
{
public:
	Canvas(CWindowGUI *cwindow,
		VWindowGUI *vwindow,
		int x, 
		int y, 
		int w, 
		int h,
		int use_scrollbars);
	virtual ~Canvas();

	void release_refresh_frame();
// Get dimensions given a zoom
	void calculate_sizes(int output_w, int output_h,
		double zoom, int &w, int &h);
// Lock access to the canvas pointer.
// Must be called before get_canvas or locking the canvas.
	void lock_canvas(const char *location);
	void unlock_canvas();
	int is_locked();

	void set_cursor(int cursor);
// Start video playback
	void start_video();
	void stop_video();

// Start single frame processing.  Causes the status indicator to update
	void start_single();
	void stop_single();

	void start_fullscreen();
	void stop_fullscreen();

// Don't call from inside the canvas
	void create_canvas();

// Processing or video playback changed.
	virtual void status_event() {};

	virtual void reset_camera() {};
	virtual void reset_projector() {};
	virtual void zoom_resize_window(double percentage) {};
	virtual void zoom_auto() {};
	virtual int cursor_leave_event() { return 0; };
	virtual int cursor_enter_event() { return 0; };
	virtual int button_release_event() { return 0; };
	virtual int button_press_event();
	virtual int cursor_motion_event() { return 0; };
	virtual void draw_overlays() { };
	virtual void toggle_controls() { } ;
	virtual int get_cwindow_controls() { return 0; };
	virtual int get_fullscreen() { return 0; }
	virtual void set_fullscreen(int value) { };
	virtual double sample_aspect_ratio() { return 1; };

	int cursor_leave_event_base(BC_WindowBase *caller);
	int cursor_enter_event_base(BC_WindowBase *caller);
	int button_press_event_base(BC_WindowBase *caller);
	int keypress_event(BC_WindowBase *caller);

// Provide canvas dimensions since a BC_Bitmap containing obsolete dimensions
// is often the output being transferred to.
// This gets the input coordinates on the device output_frame
// and the corresponding output coordinates on the canvas.
// Must be floating point to support OpenGL.
	void get_transfers(double &output_x1,
		double &output_y1,
		double &output_x2,
		double &output_y2,
		double &canvas_x1,
		double &canvas_y1,
		double &canvas_x2,
		double &canvas_y2,
// passing -1 causes automatic size detection
		int canvas_w = -1,
		int canvas_h = -1);
	void reposition_window(EDL *edl, int x, int y, int w, int h);
	virtual void reset_translation() {};
	virtual void close_source() {};
// Updates the stores
	virtual void update_zoom(int x, int y, double zoom) {};
	void update_scrollbars();
// Get scrollbar positions relative to output.
// No correction is done if output is smaller than canvas
	virtual int get_xscroll() { return 0; };
	virtual int get_yscroll() { return 0; };
	virtual double get_zoom() { return 0; };
// Redraws the image
	void draw_refresh();

	void get_zooms(EDL *edl,
		double &zoom_x,
		double &zoom_y,
		double &conformed_w,
		double &conformed_h);

// Convert coord from output to canvas position, including
// x and y scroll offsets
	void output_to_canvas(double &x, double &y);

// Get if scrollbars exist
	int scrollbars_exist();
// Get cursor locations relative to canvas and not offset by scrollbars
	int get_cursor_x();
	int get_cursor_y();
	int get_buttonpress();
// Gets whatever video surface is enabled
	BC_WindowBase* get_canvas();
// Clear the canvas
	void clear_canvas(int do_flash = 0);

// The owner of the canvas
	BC_WindowBase *subwindow;
// Video surface if a subwindow
	CanvasOutput *canvas_subwindow;
// Video surface if fullscreen
	CanvasFullScreen *canvas_fullscreen;
	CanvasXScroll *xscroll;
	CanvasYScroll *yscroll;
	CanvasPopup *canvas_menu;
	CanvasFullScreenPopup *fullscreen_menu;
	int x, y, w, h;
	int use_scrollbars;
// Last frame played is stored here in driver format for 
// refreshes.
	VFrame *refresh_frame;
// Results from last get_scrollbars
	int w_needed;
	int h_needed;
	int w_visible;
	int h_visible;
// For cases where video is not enabled on the canvas but processing is 
// occurring for a single frame, this causes the status to update.
	int is_processing;
// Cursor is inside video surface
	int cursor_inside;
	int view_x;
	int view_y;
	int view_w;
	int view_h;
	int root_w;
	int root_h;

	CWindowGUI *cwindowgui;
	VWindowGUI *vwindowgui;
	GuideLines guidelines;

protected:
// Get top left offset of canvas relative to output.
// Normally negative.  Can be positive if output is smaller than canvas.
	double get_x_offset(double zoom_x,
		double conformed_w,
		double conformed_h);
	double get_y_offset(double zoom_y,
		double conformed_w,
		double conformed_h);
	void get_zooms(double &zoom_x,
		double &zoom_y,
		double &conformed_w,
		double &conformed_h);

// Convert coord from canvas to output position
	void canvas_to_output(double &x, double &y);

private:
	void get_scrollbars(int &canvas_x, int &canvas_y,
		int &canvas_w, int &canvas_h);
// Get dimensions of frame being sent to canvas
	int get_output_w();
	int get_output_h();

	Mutex *canvas_lock;
	EDL *edl;
};


class CanvasOutput : public BC_SubWindow
{
public:
	CanvasOutput(Canvas *canvas,
		int x,
		int y,
		int w,
		int h);

	void cursor_leave_event();
	int cursor_enter_event();
	int button_press_event();
	int button_release_event();
	int cursor_motion_event();
	int keypress_event();
	void repeat_event(int duration);

	Canvas *canvas;
};


class CanvasFullScreen : public BC_FullScreen
{
public:
	CanvasFullScreen(Canvas *canvas,
		int w,
		int h);

	Canvas *canvas;
};


class CanvasXScroll : public BC_ScrollBar
{
public:
	CanvasXScroll(EDL *edl, 
		Canvas *canvas, 
		int x, 
		int y, 
		int length, 
		int position, 
		int handle_length, 
		int pixels);

	int handle_event();
	Canvas *canvas;
	EDL *edl;
};


class CanvasYScroll : public BC_ScrollBar
{
public:
	CanvasYScroll(EDL *edl, 
		Canvas *canvas, 
		int x, 
		int y, 
		int length, 
		int position, 
		int handle_length, 
		int pixels);

	int handle_event();

	Canvas *canvas;
	EDL *edl;
};


class CanvasFullScreenPopup : public BC_PopupMenu
{
public:
	CanvasFullScreenPopup(Canvas *canvas);
};


class CanvasSubWindowItem : public BC_MenuItem
{
public:
	CanvasSubWindowItem(Canvas *canvas);

	int handle_event();
	Canvas *canvas;
};


class CanvasPopup : public BC_PopupMenu
{
public:
	CanvasPopup(Canvas *canvas);
};


class CanvasPopupSize : public BC_MenuItem
{
public:
	CanvasPopupSize(Canvas *canvas, char *text, double percentage);

	int handle_event();
	Canvas *canvas;
	double percentage;
};


class CanvasPopupAuto : public BC_MenuItem
{
public:
	CanvasPopupAuto(Canvas *canvas);
	int handle_event();
	Canvas *canvas;
};


class CanvasPopupResetCamera : public BC_MenuItem
{
public:
	CanvasPopupResetCamera(Canvas *canvas);
	int handle_event();
	Canvas *canvas;
};


class CanvasPopupResetProjector : public BC_MenuItem
{
public:
	CanvasPopupResetProjector(Canvas *canvas);
	int handle_event();
	Canvas *canvas;
};


class CanvasToggleControls : public BC_MenuItem
{
public:
	CanvasToggleControls(Canvas *canvas);
	int handle_event();

private:
	static const char* calculate_text();
	Canvas *canvas;
};


class CanvasFullScreenItem : public BC_MenuItem
{
public:
	CanvasFullScreenItem(Canvas *canvas);
	int handle_event();
	Canvas *canvas;
};


class CanvasPopupRemoveSource : public BC_MenuItem
{
public:
	CanvasPopupRemoveSource(Canvas *canvas);
	int handle_event();
	Canvas *canvas;
};

#endif
