#ifndef CANVAS_H
#define CANVAS_H

#include "edl.inc"
#include "guicast.h"

// Output for all X11 video

class CanvasOutput;
class CanvasXScroll;
class CanvasYScroll;
class CanvasPopup;

// The EDL arguments can be set to 0 if the canvas_w and canvas_h are used
class Canvas
{
public:
	Canvas(BC_WindowBase *subwindow, 
		int x, 
		int y, 
		int w, 
		int h,
		int output_w,
		int output_h,
		int use_scrollbars,
		int use_cwindow = 0,
		int use_rwindow = 0,
		int use_vwindow = 0); // Use menu different options for different windows
	virtual ~Canvas();

	void reset();
// Get dimensions given a zoom
	void calculate_sizes(float aspect_ratio, 
		int output_w, 
		int output_h, 
		float zoom, 
		int &w, 
		int &h);

	int create_objects(EDL *edl);
	void set_cursor(int cursor);
	virtual void reset_camera() {};
	virtual void reset_projector() {};
	virtual void zoom_resize_window(float percentage) {};
	virtual int cursor_leave_event() { return 0; };
	virtual int cursor_enter_event() { return 0; };
	virtual int button_release_event() { return 0; };
	virtual int button_press_event();
	virtual int cursor_motion_event() { return 0; };
	virtual void draw_overlays() { };
// Provide canvas dimensions since a BC_Bitmap containing obsolete dimensions
// is often the output being transferred to
	void get_transfers(EDL *edl, int &in_x, 
		int &in_y, 
		int &in_w, 
		int &in_h,
		int &out_x, 
		int &out_y, 
		int &out_w, 
		int &out_h,
		int canvas_w = -1,
		int canvas_h = -1);
	void reposition_window(EDL *edl, int x, int y, int w, int h);
	virtual void reset_translation() {};
	virtual void close_source() {};
// Updates the stores
	virtual void update_zoom(int x, int y, float zoom) {};
	void check_boundaries(EDL *edl, int &x, int &y, float &zoom);
	void update_scrollbars();
// Get scrollbar positions relative to output.
// No correction is done if output is smaller than canvas
	virtual int get_xscroll() { return 0; };
	virtual int get_yscroll() { return 0; };
	virtual float get_zoom() { return 0; };
// Redraws the image
	virtual void draw_refresh() {};

// Get top left offset of canvas relative to output.
// Normally negative.  Can be positive if output is smaller than canvas.
	float get_x_offset(EDL *edl, 
		int single_channel, 
		float zoom_x, 
		float conformed_w,
		float conformed_h);
	float get_y_offset(EDL *edl, 
		int single_channel, 
		float zoom_y, 
		float conformed_w,
		float conformed_h);
	void get_zooms(EDL *edl, 
		int single_channel, 
		float &zoom_x, 
		float &zoom_y,
		float &conformed_w,
		float &conformed_h);
	
	
// Convert coord from output to canvas position, including
// x and y scroll offsets
	void output_to_canvas(EDL *edl, int single_channel, float &x, float &y);
// Convert coord from canvas to output position
	void canvas_to_output(EDL *edl, int single_channel, float &x, float &y);

// Get dimensions of frame being sent to canvas
	virtual int get_output_w(EDL *edl);
	virtual int get_output_h(EDL *edl);
// Get if scrollbars exist
	int scrollbars_exist();
// Get cursor locations relative to canvas and not offset by scrollbars
	int get_cursor_x();
	int get_cursor_y();
	int get_buttonpress();

	BC_WindowBase *subwindow;
	CanvasOutput *canvas;
	CanvasXScroll *xscroll;
	CanvasYScroll *yscroll;
	CanvasPopup *canvas_menu;
	int x, y, w, h;
	int use_scrollbars;
	int use_cwindow;
	int use_rwindow;
	int use_vwindow;
// Used in record monitor
	int output_w, output_h;
// Store frame in native format after playback for refreshes
	VFrame *refresh_frame;
// Results from last get_scrollbars
	int w_needed;
	int h_needed;
	int w_visible;
	int h_visible;

private:
	void get_scrollbars(EDL *edl, 
		int &canvas_x, 
		int &canvas_y, 
		int &canvas_w, 
		int &canvas_h);
};


class CanvasOutput : public BC_SubWindow
{
public:
	CanvasOutput(EDL *edl, 
	    Canvas *canvas,
        int x,
        int y,
        int w,
        int h);
	~CanvasOutput();

	int handle_event();
	int cursor_leave_event();
	int cursor_enter_event();
	int button_press_event();
	int button_release_event();
	int cursor_motion_event();

	EDL *edl;
	Canvas *canvas;
	int cursor_inside;
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
	~CanvasXScroll();

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
	~CanvasYScroll();

	int handle_event();

	Canvas *canvas;
	EDL *edl;
};

class CanvasPopup : public BC_PopupMenu
{
public:
	CanvasPopup(Canvas *canvas);
	~CanvasPopup();

	void create_objects();

	Canvas *canvas;
};

class CanvasPopupSize : public BC_MenuItem
{
public:
	CanvasPopupSize(Canvas *canvas, char *text, float percentage);
	~CanvasPopupSize();
	int handle_event();
	Canvas *canvas;
	float percentage;
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

class CanvasPopupResetTranslation : public BC_MenuItem
{
public:
	CanvasPopupResetTranslation(Canvas *canvas);
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
