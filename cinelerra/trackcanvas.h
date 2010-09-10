
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

#ifndef TRACKCANVAS_H
#define TRACKCANVAS_H

#include "asset.inc"
#include "auto.inc"
#include "autos.inc"
#include "bctimer.inc"
#include "edit.inc"
#include "edithandles.inc"
#include "floatauto.inc"
#include "floatautos.inc"
#include "guicast.h"
#include "keyframe.inc"
#include "mwindow.inc"
#include "mwindowgui.inc"
#include "resourcethread.inc"
#include "plugin.inc"
#include "plugintoggles.inc"
#include "resourcepixmap.inc"
#include "track.inc"
#include "tracks.inc"
#include "keyframe.inc"
#include "floatauto.inc"

class TrackCanvas : public BC_SubWindow
{
public:
	TrackCanvas(MWindow *mwindow, MWindowGUI *gui);
	~TrackCanvas();

	int create_objects();
	void resize_event();
	int drag_start_event();
	int drag_motion_event();
	int drag_stop_event();
// mode - 1 causes incremental drawing of pixmaps.  Used for navigation and index refresh.
//        2 causes all resource pixmaps to be redrawn from scratch.  Used by editing.
//        3 causes resource pixmaps to ignore picon thread.  Used by Piconthread.
	void draw_resources(int mode = 0,
		int indexes_only = 0,     // Redraw only certain audio resources with indexes
		Asset *index_asset = 0);
	void draw_highlight_rectangle(int x, int y, int w, int h);
	void draw_highlight_insertion(int x, int y, int w, int h);
	void draw_highlighting();
// User can either call draw or draw_overlays to copy a fresh 
// canvas and just draw the overlays over it
	void draw_overlays();
// Convert edit coords to transition coords
	void get_transition_coords(int &x, int &y, int &w, int &h);
	void get_handle_coords(Edit *edit, 
		int &x,
		int &y,
		int &w,
		int &h,
		int side);
	void draw_auto(Auto *current, 
		int x, 
		int y, 
		int center_pixel, 
		int zoom_track,
		int color);
	void draw_floatauto(Auto *current, 
		int x, 
		int y, 
		int in_x,
		int in_y,
		int out_x,
		int out_y,
		int center_pixel, 
		int zoom_track,
		int color);
	int test_auto(Auto *current, 
		int x, 
		int y, 
		int center_pixel, 
		int zoom_track, 
		int cursor_x, 
		int cursor_y, 
		int buttonpress);
	int test_floatauto(Auto *current, 
		int x, 
		int y, 
		int in_x,
		int in_y,
		int out_x,
		int out_y,
		int center_pixel, 
		int zoom_track, 
		int cursor_x, 
		int cursor_y, 
		int buttonpress);
	void draw_floatline(int center_pixel, 
		FloatAuto *previous,
		FloatAuto *current,
		FloatAutos *autos,
		double view_start,
		double xzoom,
		double yscale,
		int ax,
		int ay,
		int ax2,
		int ay2,
		int color,
		int autogrouptype);
	int test_floatline(int center_pixel, 
		FloatAutos *autos,
		double view_start,
		double xzoom,
		double yscale,
		int x1,
		int x2,
		int cursor_x, 
		int cursor_y, 
		int buttonpress,
		int autogrouptype);
	void draw_toggleline(int center_pixel, 
		int ax,
		int ay,
		int ax2,
		int ay2,
		int color);
	int test_toggleline(Autos *autos,
		int center_pixel, 
		int x1,
		int y1,
		int x2,
		int y2,
		int cursor_x, 
		int cursor_y, 
		int buttonpress);
	int do_keyframes(int cursor_x, 
		int cursor_y, 
		int draw, 
		int buttonpress, 
		int &new_cursor,
		int &update_cursor,
		int &rerender);

	int do_float_autos(Track *track, 
		Autos *autos, 
		int cursor_x, 
		int cursor_y, 
		int draw, 
		int buttonpress,
		int color,
		Auto * &auto_instance,
		int autogrouptype);
	int do_toggle_autos(Track *track, 
		Autos *autos, 
		int cursor_x, 
		int cursor_y, 
		int draw, 
		int buttonpress,
		int color,
		Auto * &auto_instance);
	int do_autos(Track *track, 
		Autos *autos, 
		int cursor_x, 
		int cursor_y, 
		int draw, 
		int buttonpress,
		BC_Pixmap *pixmap,
		Auto * &auto_instance);
	int do_plugin_autos(Track *track,
		int cursor_x, 
		int cursor_y, 
		int draw, 
		int buttonpress,
		Plugin* &keyframe_plugin,
		KeyFrame* &keyframe_instance);


	void calculate_viewport(Track *track, 
		ptstime &view_start,
		ptstime &view_end,
		double &yscale,
		double &xzoom,
		int &center_pixel);

// Convert percentage position inside track to value.
// if is_toggle is 1, the result is either 0 or 1.
// if reference is nonzero and a FloatAuto, 
//     the result is made relative to the value in reference.
	float percentage_to_value(float percentage, 
		int is_toggle,
		Auto *reference,
		int autogrouptype);

// Get x and y of a FloatAuto relative to center_pixel
	void calculate_auto_position(double *x, 
		double *y,
		double *in_x,
		double *in_y,
		double *out_x,
		double *out_y,
		Auto *current,
		ptstime start,
		double zoom,
		double yscale,
		int autogrouptype);
	void synchronize_autos(float change, Track *skip, FloatAuto *fauto, int fill_gangs);

	void draw_brender_start();
	void draw_loop_points();
	void draw_transitions();
	void draw_drag_handle();
	void draw_plugins();
	void refresh_plugintoggles();

// Draw everything to synchronize with the view.
// mode - if 2 causes all resource pixmaps to be redrawn from scratch
//        if 3 causes resource pixmaps to ignore picon thread
	void draw(int mode = 0, int hide_cursor = 1);

// Draw resources during index building
	void draw_indexes(Asset *asset);

// Get location of edit on screen without boundary checking
	void edit_dimensions(Edit *edit, int &x, int &y, int &w, int &h);
	void track_dimensions(Track *track, int &x, int &y, int &w, int &h);
	void plugin_dimensions(Plugin *plugin, int &x, int &y, int &w, int &h);
	void get_pixmap_size(Edit *edit, int edit_x, int edit_w, int &pixmap_x, int &pixmap_w, int &pixmap_h);
	ResourcePixmap* create_pixmap(Edit *edit, int pixmap_w, int pixmap_h);
	void update_cursor();

// Get edit and handle the cursor is over
	int do_edit_handles(int cursor_x, 
		int cursor_y, 
		int button_press,
		int &rerender,
		int &update_overlay,
		int &new_cursor,
		int &update_cursor);
// Get plugin and handle the cursor if over
	int do_plugin_handles(int cursor_x, 
		int cursor_y, 
		int button_press,
		int &rerender,
		int &update_overlay,
		int &new_cursor,
		int &update_cursor);
// Get edit the cursor is over
	int do_edits(int cursor_x, 
		int cursor_y, 
		int button_press,
		int drag_start,
		int &redraw,
		int &rerender,
		int &new_cursor,
		int &update_cursor);
	int do_tracks(int cursor_x, 
		int cursor_y,
		int button_press);
	int do_plugins(int cursor_x, 
		int cursor_y, 
		int drag_start,
		int button_press,
		int &redraw,
		int &rerender);
	int do_transitions(int cursor_x, 
		int cursor_y, 
		int button_press,
		int &new_cursor,
		int &update_cursor);
	int button_press_event();
	int button_release_event();
	int cursor_motion_event();
	int activate();
	int deactivate();
	int repeat_event(int duration);
	void start_dragscroll();
	void stop_dragscroll();
	int start_selection(double position);
	int drag_motion();
	int drag_stop();
	posnum get_drop_position (int *is_insertion, Edit *moved_edit, 
		posnum moved_edit_length);
	void end_edithandle_selection();
	void end_pluginhandle_selection();
// Number of seconds spanned by the trackcanvas
	double time_visible();
	void update_drag_handle();
	int update_drag_floatauto(int cursor_x, int cursor_y);
	int update_drag_toggleauto(int cursor_x, int cursor_y);
	int update_drag_auto(int cursor_x, int cursor_y);
	int update_drag_pluginauto(int cursor_x, int cursor_y);

	int resource_h();

// Display hourglass if timer expired
	void test_timer();

	MWindow *mwindow;
	MWindowGUI *gui;
	ArrayList<ResourcePixmap*> resource_pixmaps;
// Allows overlays to get redrawn without redrawing the resources
	BC_Pixmap *background_pixmap;
	BC_DragWindow *drag_popup;
	BC_Pixmap *transition_pixmap;
	EditHandles *edit_handles;
	BC_Pixmap *keyframe_pixmap;
	BC_Pixmap *camerakeyframe_pixmap;
	BC_Pixmap *modekeyframe_pixmap;
	BC_Pixmap *pankeyframe_pixmap;
	BC_Pixmap *projectorkeyframe_pixmap;
	BC_Pixmap *maskkeyframe_pixmap;

	int active;
// Currently in a drag scroll operation
	int drag_scroll;
// Don't stop hourglass if it was never started before the operation.
	int hourglass_enabled;

// Temporary for picon drawing
	VFrame *temp_picon;
// Timer for hourglass
	Timer *resource_timer;

// Plugin toggle interfaces
	ArrayList<PluginOn*> plugin_on_toggles;
	ArrayList<PluginShow*> plugin_show_toggles;

	ResourceThread *resource_thread;

	void draw_paste_destination();

private:
// ====================================== cursor selection type
	int auto_selected;               // 1 if automation selected
	int translate_selected;          // 1 if video translation selected

	int handle_selected;	// if a handle is selected
				// 1 if not floating yet
				// 2 if floating
	int which_handle;	// 1 left or 2 right handle
	double selection_midpoint1, selection_midpoint2;        // division between current ends
	int region_selected;	// 1 if region selected
};

#endif
