#ifndef VWINDOWGUI_H
#define VWINDOWGUI_H

#include "asset.inc"
#include "canvas.h"
#include "editpanel.h"
#include "guicast.h"
#include "mainclock.inc"
#include "meterpanel.h"
#include "mwindow.inc"
#include "playtransport.h"

#include "timebar.h"

#include "vtimebar.inc"
#include "vwindow.inc"
#include "zoompanel.h"

class VWindowSlider;
class VWindowZoom;
class VWindowSource;
class VWindowTransport;
class VWindowEditing;
class VWindowCanvas;
class VWindowMeters;
class VWindowInPoint;
class VWindowOutPoint;

class VWindowGUI : public BC_Window
{
public:
	VWindowGUI(MWindow *mwindow, VWindow *vwindow);
	~VWindowGUI();

	int create_objects();
	int resize_event(int w, int h);
	int translation_event();
	int close_event();
	int keypress_event();
// Update source pulldown with new assets
	void update_sources(char *title);
// Update GUI to reflect new source
	void change_source(EDL *edl, char *title);
	void drag_motion();
	int drag_stop();
//	void update_labels();
//	void update_points();

	MWindow *mwindow;
	VWindow *vwindow;

// Meters are numbered from right to left
	VWindowCanvas *canvas;
	VWindowSlider *slider;
	BC_Title *fps_title;
	MainClock *clock;
	VTimeBar *timebar;
	VWindowZoom *zoom_panel;
	VWindowTransport *transport;
	VWindowEditing *edit_panel;
//	VWindowSource *source;
	VWindowMeters *meters;
	ArrayList<BC_ListBoxItem*> sources;
	ArrayList<LabelGUI*> labels;
	VWindowInPoint *in_point;
	VWindowOutPoint *out_point;

private:
	void get_scrollbars(int &canvas_x, int &canvas_y, int &canvas_w, int &canvas_h);
};


class VWindowMeters : public MeterPanel
{
public:
	VWindowMeters(MWindow *mwindow, VWindowGUI *gui, int x, int y, int h);
	~VWindowMeters();
	
	int change_status_event();
	
	MWindow *mwindow;
	VWindowGUI *gui;
};


class VWindowCanvas : public Canvas
{
public:
	VWindowCanvas(MWindow *mwindow, VWindowGUI *gui);

	void zoom_resize_window(float percentage);
	void draw_refresh();
	void draw_overlays();
	void close_source();

	MWindow *mwindow;
	VWindowGUI *gui;
};

class VWindowEditing : public EditPanel
{
public:
	VWindowEditing(MWindow *mwindow, VWindow *vwindow);
	~VWindowEditing();
	
	void copy_selection();
	void splice_selection();
	void overwrite_selection();
	void set_inpoint();
	void set_outpoint();
	void clear_inpoint();
	void clear_outpoint();
	void to_clip();
	void toggle_label();
	void prev_label();
	void next_label();

	MWindow *mwindow;
	VWindow *vwindow;
};

class VWindowZoom : public ZoomPanel
{
public:
	VWindowZoom(MWindow *mwindow, VWindowGUI *gui, int x, int y);
	~VWindowZoom();
	int handle_event();
	MWindow *mwindow;
	VWindowGUI *gui;
};


class VWindowSlider : public BC_PercentageSlider
{
public:
	VWindowSlider(MWindow *mwindow, 
		VWindow *vwindow, 
		VWindowGUI *gui, 
		int x, 
		int y, 
		int pixels);
	~VWindowSlider();

	int handle_event();
	void set_position();

	VWindowGUI *gui;
	MWindow *mwindow;
	VWindow *vwindow;
};

class VWindowSource : public BC_PopupTextBox
{
public:
	VWindowSource(MWindow *mwindow, VWindowGUI *vwindow, int x, int y);
	~VWindowSource();
	int handle_event();
	VWindowGUI *vwindow;
	MWindow *mwindow;
};

class VWindowTransport : public PlayTransport
{
public:
	VWindowTransport(MWindow *mwindow, 
		VWindowGUI *gui, 
		int x, 
		int y);
	EDL* get_edl();
	void goto_start();
	void goto_end();

	VWindowGUI *gui;
};

class VWindowInPoint : public InPointGUI
{
public:
	VWindowInPoint(MWindow *mwindow, 
		TimeBar *timebar, 
		VWindowGUI *gui,
		long pixel, 
		double position);
	int handle_event();
	VWindowGUI *gui;
};


class VWindowOutPoint : public OutPointGUI
{
public:
	VWindowOutPoint(MWindow *mwindow, 
		TimeBar *timebar, 
		VWindowGUI *gui,
		long pixel, 
		double position);
	int handle_event();
	VWindowGUI *gui;
};

#endif
