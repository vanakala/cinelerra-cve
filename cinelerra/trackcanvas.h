#ifndef TRACKCANVAS_H
#define TRACKCANVAS_H

#include "asset.inc"
#include "auto.inc"
#include "autos.inc"
#include "bezierauto.inc"
#include "edit.inc"
#include "edithandles.inc"
#include "floatauto.inc"
#include "floatautos.inc"
#include "guicast.h"
#include "mwindow.inc"
#include "mwindowgui.inc"
#include "plugin.inc"
#include "resourcepixmap.inc"
#include "track.inc"
#include "tracks.inc"
#include "transitionhandles.inc"

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
	int keypress_event();
	void draw_resources(int force = 0,  // Redraw everything
		int indexes_only = 0,     // Redraw only certain audio resources with indexes
		Asset *index_asset = 0);
	void draw_highlight_rectangle(int x, int y, int w, int h);
	void draw_playback_cursor();
	void draw_highlighting();
// User can either call draw or draw_overlays to copy a fresh 
// canvas and just draw the overlays over it
	void draw_overlays();
	void update_handles();
// Convert edit coords to transition coords
	void get_transition_coords(int64_t &x, int64_t &y, int64_t &w, int64_t &h);
	void get_handle_coords(Edit *edit, 
		int64_t &x, 
		int64_t &y, 
		int64_t &w, 
		int64_t &h, 
		int side);
	void draw_title(Edit *edit, 
		int64_t edit_x, 
		int64_t edit_y, 
		int64_t edit_w, 
		int64_t edit_h);
	void draw_automation();
	void draw_inout_points();
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
		int64_t unit_start,
		double zoom_units,
		double yscale,
		int ax,
		int ay,
		int ax2,
		int ay2,
		int color);
	int test_floatline(int center_pixel, 
		FloatAutos *autos,
		int64_t unit_start,
		double zoom_units,
		double yscale,
		int x1,
		int x2,
		int cursor_x, 
		int cursor_y, 
		int buttonpress);
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
		int color);
	int do_toggle_autos(Track *track, 
		Autos *autos, 
		int cursor_x, 
		int cursor_y, 
		int draw, 
		int buttonpress,
		int color);
	int do_autos(Track *track, 
		Autos *autos, 
		int cursor_x, 
		int cursor_y, 
		int draw, 
		int buttonpress,
		BC_Pixmap *pixmap);
	int do_plugin_autos(Track *track,
		int cursor_x, 
		int cursor_y, 
		int draw, 
		int buttonpress);


	void calculate_viewport(Track *track, 
		double &view_start,
		int64_t &unit_start,
		double &view_end,
		int64_t &unit_end,
		double &yscale,
		int &center_pixel,
		double &zoom_sample,
		double &zoom_units);

	void draw_brender_start();
	void draw_loop_points();
	void draw_transitions();
	void draw_drag_handle();
	void draw_plugins();
	void update_edit_handles(Edit *edit, int64_t edit_x, int64_t edit_y, int64_t edit_w, int64_t edit_h);
	void update_transitions();
	void update_keyframe_handles(Track *track);
	void get_keyframe_sizes(Track *track, BezierAuto *current, int64_t &x, int64_t &y, int64_t &w, int64_t &h);
// Draw everything to synchronize with the view
	void draw(int force = 0, int hide_cursor = 1);
// Draw resources during index building
	void draw_indexes(Asset *asset);
// Get location of edit on screen without boundary checking
	void edit_dimensions(Edit *edit, int64_t &x, int64_t &y, int64_t &w, int64_t &h);
	void track_dimensions(Track *track, int64_t &x, int64_t &y, int64_t &w, int64_t &h);
	void plugin_dimensions(Plugin *plugin, int64_t &x, int64_t &y, int64_t &w, int64_t &h);
	void get_pixmap_size(Edit *edit, int64_t edit_x, int64_t edit_w, int64_t &pixmap_x, int64_t &pixmap_w, int64_t &pixmap_h);
	ResourcePixmap* create_pixmap(Edit *edit, int64_t edit_x, int64_t pixmap_x, int64_t pixmap_w, int64_t pixmap_h);
	int set_index_file(int flash, Asset *asset);
	void update_cursor();
// Get edit and handle the cursor is over
	int test_edit_handles(int cursor_x, 
		int cursor_y, 
		int button_press,
		int &redraw,
		int &rerender);
// Get plugin and handle the cursor if over
	int test_plugin_handles(int cursor_x, 
		int cursor_y, 
		int button_press,
		int &redraw,
		int &rerender);
// Get edit the cursor is over
	int test_edits(int cursor_x, 
		int cursor_y, 
		int button_press,
		int drag_start,
		int &redraw,
		int &rerender,
		int &new_cursor,
		int &update_cursor);
	int test_tracks(int cursor_x, 
		int cursor_y,
		int button_press);
	int test_resources(int cursor_x, int cursor_y);
	int test_plugins(int cursor_x, 
		int cursor_y, 
		int drag_start,
		int button_press);
	int test_transitions(int cursor_x, 
		int cursor_y, 
		int button_press,
		int &new_cursor,
		int &update_cursor);
	int button_press_event();
	int button_release_event();
	int cursor_motion_event();
	int activate();
	int deactivate();
	int repeat_event(int64_t duration);
	void start_dragscroll();
	void stop_dragscroll();
	int start_selection(double position);
	int drag_motion();
	int drag_stop();
	void end_edithandle_selection();
	void end_pluginhandle_selection();
// Number of seconds spanned by the trackcanvas
	double time_visible();
	void update_drag_handle();
	int update_drag_edit();
	int update_drag_floatauto(int cursor_x, int cursor_y);
	int update_drag_toggleauto(int cursor_x, int cursor_y);
	int update_drag_auto(int cursor_x, int cursor_y);

// Update status bar to reflect drag operation
	void update_drag_caption();

	int get_title_h();
	int resource_h();

	MWindow *mwindow;
	MWindowGUI *gui;
	ArrayList<ResourcePixmap*> resource_pixmaps;
// Allows overlays to get redrawn without redrawing the resources
	BC_Pixmap *background_pixmap;
	BC_Pixmap *drag_pixmap;
	BC_DragWindow *drag_popup;
	BC_Pixmap *transition_pixmap;
	EditHandles *edit_handles;
//	TransitionHandles *transition_handles;
	BC_Pixmap *keyframe_pixmap;
	BC_Pixmap *camerakeyframe_pixmap;
	BC_Pixmap *modekeyframe_pixmap;
	BC_Pixmap *pankeyframe_pixmap;
	BC_Pixmap *projectorkeyframe_pixmap;
	BC_Pixmap *maskkeyframe_pixmap;
	int active;
// Currently in a drag scroll operation
	int drag_scroll;
















// event handlers
	int button_release();
	int draw_playback_cursor(int pixel, int flash = 1);
	int draw_loop_point(int64_t position, int flash);
	void draw_paste_destination();

	int draw_floating_handle(int flash);


private:
	int end_translation();

// ====================================== cursor selection type
	int auto_selected;               // 1 if automation selected
	int translate_selected;          // 1 if video translation selected

	int handle_selected;       // if a handle is selected
								// 1 if not floating yet
								// 2 if floating
	int which_handle;           // 1 left or 2 right handle
	int64_t handle_oldposition;       // original position of handle
	int64_t handle_position;           // current position of handle
	int handle_pixel;                // original pixel position of pointer in window
	int handle_mode;   // Determined by which button was pressed

	int current_end;       // end of selection 1 left 2 right
	double selection_midpoint1, selection_midpoint2;        // division between current ends
	int region_selected;         // 1 if region selected
	int selection_type;  // Whether an edit or a sample is selected

	int auto_reposition(int &cursor_x, int &cursor_y, int64_t cursor_position);
	int update_selection(int64_t cursor_position);
	int update_handle_selection(int64_t cursor_position);
};

#endif
