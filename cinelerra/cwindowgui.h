#ifndef CWINDOWGUI_H
#define CWINDOWGUI_H

//#include "apanel.inc"
#include "auto.inc"
#include "bezierauto.inc"
#include "bezierautos.inc"
#include "canvas.h"
#include "cpanel.inc"
#include "ctimebar.inc"
#include "cwindow.inc"
#include "cwindowtool.inc"
#include "editpanel.h"
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
	int close_event();
	int keypress_event();
	int translation_event();
	void set_operation(int value);
	void update_tool();
	void drag_motion();
	int drag_stop();

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
//	MainClock *clock;


	CWindowMeters *meters;


	CWindowTool *tool_panel;

// Cursor motion modification being done
	int current_operation;
// The pointers are only used for dragging.  Information for tool windows
// must be integers and recalculted for every keypress.
// Track being modified in canvas
	Track *affected_track;
// Keyframe being modified
	Auto *affected_auto;
// Mask point being modified
	int affected_point;
// Scrollbar offsets during last button press relative to output
	float x_offset, y_offset;
// Cursor location during the last button press relative to output
// and offset by scroll bars
	float x_origin, y_origin;
// Crop handle being dragged
	int crop_handle;
// Origin of crop handle during last button press
	float crop_origin_x, crop_origin_y;
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

	void zoom_resize_window(float percentage);
	void update_zoom(int x, int y, float zoom);
	int get_xscroll();
	int get_yscroll();
	float get_zoom();
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
	int do_bezier_center(BezierAuto *current, 
		BezierAutos *camera_autos,
		BezierAutos *projector_autos, 
		FloatAutos *czoom_autos,
		FloatAutos *pzoom_autos,
		int camera, 
		int draw);
	void draw_bezier_joining(BezierAuto *first, 
		BezierAuto *last, 
		BezierAutos *camera_autos,
		BezierAutos *projector_autos, 
		FloatAutos *czoom_autos,
		FloatAutos *pzoom_autos,
		int camera);
	void draw_bezier(int do_camera);
	void draw_crop();
	void calculate_origin();
	void toggle_controls();
	int get_cwindow_controls();

	MWindow *mwindow;
	CWindowGUI *gui;
};

#endif
