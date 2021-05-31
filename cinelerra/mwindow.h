// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef MWINDOW_H
#define MWINDOW_H

#include "arraylist.h"
#include "asset.inc"
#include "audiodevice.inc"
#include "auto.inc"
#include "autos.inc"
#include "awindow.inc"
#include "batchrender.inc"
#include "bcwindowbase.inc"
#include "brender.inc"
#include "cache.inc"
#include "clipedit.inc"
#include "cwindow.inc"
#include "bchash.inc"
#include "datatype.h"
#include "edit.inc"
#include "edl.inc"
#include "edlsession.inc"
#include "exportedl.inc"
#include "filesystem.inc"
#include "filexml.inc"
#include "framecache.inc"
#include "glthread.inc"
#include "gwindow.inc"
#include "levelwindow.inc"
#include "loadmode.inc"
#include "mainerror.inc"
#include "mainindexes.inc"
#include "mainprogress.inc"
#include "mainsession.inc"
#include "mainundo.inc"
#include "mutex.inc"
#include "mwindow.inc"
#include "mwindowgui.inc"
#include "new.inc"
#include "patchbay.inc"
#include "playbackengine.inc"
#include "plugin.inc"
#include "pluginclient.inc"
#include "pluginserver.inc"
#include "preferences.inc"
#include "preferencesthread.inc"
#include "render.inc"
#include "ruler.inc"
#include "sighandler.inc"
#include "splashgui.inc"
#include "theme.inc"
#include "thread.h"
#include "timebar.inc"
#include "tipwindow.inc"
#include "track.inc"
#include "tracking.inc"
#include "tracks.inc"
#include "transportcommand.inc"
#include "vframe.inc"
#include "vwindow.inc"
#include "wavecache.inc"

#include <stdint.h>

// All entry points for commands except for window locking should be here.
// This allows scriptability.

class MWindow : public Thread
{
public:
	MWindow(const char *config_path);
	~MWindow();

// ======================================== initialization commands
	void show_splash();
	void hide_splash();
	void start();
	void run();

	int new_project();
	int delete_project(int flash = 1);

	int load_defaults();
	void save_defaults();
	void set_filename(const char *filename);
// Add path to recetly loaded files
	void add_load_path(const char *path);
// Total vertical pixels in timeline
	int get_tracks_height();
// Total horizontal pixels in timeline
	int get_tracks_width();
// Show windows
	void show_vwindow();
	void show_awindow();
	void show_lwindow();
	void show_cwindow();
	void show_gwindow();
	void show_ruler();
	void tile_windows();
// Gui functions
	void show_message(const char *fmt, ...);
	void default_message();
	void change_meter_format(int min_db, int max_db);
	void update_undo_text(const char *text);
	void update_redo_text(const char *text);
	void add_aeffect_menu(const char *title);
	void add_veffect_menu(const char *title);

// Mark window hidden
	void mark_awindow_hidden();
	void mark_cwindow_hidden();
	void mark_gwindow_hidden();
	void mark_lwindow_hidden();
	void mark_ruler_hidden();
	void mark_vwindow_hidden();

// Entry point to insert assets and insert edls.  Called by TrackCanvas 
// and AssetPopup when assets are dragged in from AWindow.
// Takes the drag vectors from MainSession and
// pastes either assets or clips depending on which is full.
	void paste_assets(ptstime position, Track *dest_track, int overwrite);

// Insert the assets at a point in the EDL.  Called by menueffects,
// render, and CWindow drop but recording calls paste_edls directly for
// labels.
	void load_assets(ArrayList<Asset*> *new_assets, 
		ptstime position,
		Track *first_track,
		int overwrite);
	ptstime paste_edl(EDL *new_edl,
		int load_mode,
		Track *first_track,
		ptstime current_position,
		int overwrite);
	void paste_edls(ArrayList<EDL*> *new_edls, 
		int load_mode, 
		Track *first_track,
		ptstime current_position,
		int overwrite);
// Reset everything for a load
	void update_project(int load_mode);
// Fit selected time to horizontal display range
	void fit_selection(void);
// Fit selected autos to the vertical display range
	void fit_autos(int doall);
	void change_currentautorange(int autogrouptype, int increment, int changemax);
	void expand_autos(int changeall, int domin, int domax);
	void shrink_autos(int changeall, int domin, int domax);
// move the window to include the cursor
	void find_cursor(void);

	void load_filenames(ArrayList<char*> *filenames,
		int load_mode = LOADMODE_REPLACE);

	void interrupt_indexes();  // Stop index building

	void draw_indexes(Asset *asset);

// options (bits):
//          WUPD_SCROLLBARS - update scrollbars
//          WUPD_CANVINCR   - incremental drawing of resources
//          WUPD_CANVREDRAW - delete and redraw of resources
//          WUPD_TIMEBAR    - update timebar
//          WUPD_ZOOMBAR    - update zoombar
//          WUPD_PATCHBAY   - update patchbay
//          WUPD_CLOCK      - update clock
//          WUPD_BUTTONBAR  - update buttonbar
//          WUPD_TOGGLES    - update toggles
//          WUPD_LABELS     - update labels
//          WUPD_TIMEDEPS   - update time dependencies of zoombar
//          WUPD_CURSOR     - update cursor
//          WUPD_TOGLIGHTS  - update highlights of timebar toggles
	void update_gui(int options, ptstime new_position = -1);
	void start_hourglass();
	void stop_hourglass();

// =========================================== movement

	void next_time_format();
	void prev_time_format();
	void time_format_common();
	int reposition_timebar(int new_pixel, int new_height);
	void expand_sample(void);
	void zoom_in_sample(void);
	void zoom_time(ptstime zoom);
	void zoom_amp(int zoom_amp);
	void zoom_track(int zoom_track);
	int fit_sample();
	void move_left(int distance = 0);
	void move_right(int distance = 0);
	void move_up(int distance = 0);
	void move_down(int distance = 0);

// seek to labels
// shift_down must be passed by the caller because different windows call
// into this
	void next_label(int shift_down);
	void prev_label(int shift_down);
// seek to edit handles
	void next_edit_handle(int shift_down);
	void prev_edit_handle(int shift_down);
	void trackmovement(int track_start);
	void samplemovement(ptstime view_start, int force_redraw = 0);
	void select_all(void);
	void goto_start();
	void goto_end();
	void expand_y(void);
	void zoom_in_y(void);
	void expand_t(void);
	void zoom_in_t(void);
// Call after every edit operation
	void save_backup(int is_manual = 0);
	void show_plugin(Plugin *plugin);
// Update plugins with configuration changes.
// Called by TrackCanvas::cursor_motion_event.
	void update_plugin_guis();

// ============================= editing commands ========================

// Map each recordable audio track to the desired pattern
	void map_audio(int pattern);
	enum
	{
		AUDIO_5_1_TO_2,
		AUDIO_1_TO_1
	};
	void add_track(int TRACK_TYPE, int above = 0, Track *dst = 0);
	void add_clip_to_edl(EDL *edl);

	void asset_to_size(Asset *asset);
	void asset_to_rate(Asset *asset);

// Entry point for clear operations.
	void clear_entry();
// Clears active region in EDL.
// If clear_handle, edit boundaries are cleared if the range is 0.
// Called by paste, record, menueffects, render, and CWindow drop.
	void clear(int clear_handle);
	void clear_labels();
	void concatenate_tracks();
	void copy();
	void copy(ptstime start, ptstime end);
	void cut();

// Calculate defaults path
	static void create_defaults_path(char *string);
// Compose standard window title
	static char *create_title(const char *name);

	void delete_track();
	void delete_track(Track *track);
	void delete_tracks();
	void detach_transition(Plugin *transition);
	void insert(ptstime position, 
		FileXML *file,
		EDL *parent_edl = 0);

// TrackCanvas calls this to insert multiple effects from the drag_pluginservers
	void insert_effects_canvas(ptstime start,
		ptstime length);

// CWindow calls this to insert multiple effects from 
// the drag_pluginservers array.
	void insert_effects_cwindow(Track *dest_track);

// This is called multiple times by the above functions.
// It can't sync parameters.
	void insert_effect(const char *title, 
		Track *track,
		ptstime start,
		ptstime length,
		int plugin_type,
		Plugin *shared_plugin = 0,
		Track *shared_track = 0);

	void match_output_size(Track *track);
	void match_asset_size(Track *track);
// Move edit to new position
	void move_edits(ArrayList<Edit*> *edits,
		Track *track,
		double position,
		int behaviour);       // behaviour: 0 - old style (cut and insert elswhere), 1- new style - (clear and overwrite elsewere)
// Move effect to position
	void move_effect(Plugin *plugin,
		Track *track,
		ptstime position);
	void move_plugin_up(Plugin *plugin);
	void move_plugin_down(Plugin *plugin);
	void move_track_down(Track *track);
	void move_tracks_down();
	void move_track_up(Track *track);
	void move_tracks_up();
	void mute_selection();
	void overwrite(EDL *source);
// For clipboard commands
	void paste();

	void paste_silence();

	void paste_transition();
	void insert_transition(PluginServer *server, Edit *dst_edit);
	void paste_transition(int data_type, PluginServer *server = 0, int firstonly = 0);
	void rebuild_indices();
// Asset removal from caches
	void reset_caches();
	void remove_asset_from_caches(Asset *asset);
	void remove_assets_from_project(int push_undo = 0);
	void resize_track(Track *track, int w, int h);
	void set_auto_keyframes(int value);
	void set_labels_follow_edits(int value);

// Update the editing mode
	void set_editing_mode(int new_editing_mode);
	void toggle_editing_mode();
	void set_inpoint();
	void set_outpoint();
	void splice(EDL *source);
	void toggle_loop_playback();
	void trim_selection();
// Synchronize EDL settings with all playback engines depending on current 
// operation.  Doesn't redraw anything.
	void sync_parameters(int brender_restart = 1);
	void to_clip();
	void toggle_label();
	void undo_entry();
	void redo_entry();

	void cut_effects();
	void copy_effects();
	void paste_effects(int operation);
	void copy_keyframes(Autos *autos, Auto *keyframe, Plugin *plugin);
	void paste_keyframe(Track *track, Plugin *plugin);
	int can_paste_keyframe(Track *track, Plugin *plugin);
	void clear_keyframes(Plugin *plugin);
	void clear_automation();
	void straighten_automation();

	void modify_edithandles(void);
	void modify_pluginhandles(void);
	void finish_modify_handles(int upd_option = 0);

// Send new EDL to caches
	void age_caches();
	int optimize_assets();            // delete unused assets from the cache and assets

	void select_point(ptstime position);
	void set_loop_boundaries();         // toggle loop playback and set boundaries for loop playback
	int stop_composer();             // stop composer playback
	int stop_playback();             // stop viewer and composer playback
	// Get absolute cursor position
	void get_abs_cursor_pos(int *abs_x, int *abs_y);
	// Get icon for window
	VFrame *get_window_icon();
	// Draw trackcanvas overlays
	void draw_canvas_overlays();
	// Length of visible trackcanvas
	ptstime trackcanvas_visible();

	void activate_canvas();
	void deactivate_canvas();

	SplashGUI *splash_window;
	GLThread *glthread;

// Main undo stack
	MainUndo *undo;
	BC_Hash *defaults;
	Preferences *preferences;
	PreferencesThread *preferences_thread;
	Theme *theme;
	MainIndexes *mainindexes;
	MainProgress *mainprogress;
	BRender *brender;
	const char *default_standard;

	void reset_meters();
// Dumps program status to stdout
	void show_program_status();

	BatchRenderThread *batch_render;
	Render *render;

	ExportEDL *exportedl;

// Main Window GUI
	MWindowGUI *gui;
// Compositor
	CWindow *cwindow;
// Viewer
	VWindow *vwindow;
// Asset manager
	AWindow *awindow;
// Automation window
	GWindow *gwindow;
// Tip of the day
	TipWindow *twindow;
// Levels
	LevelWindow *lwindow;
// Ruler
	Ruler *ruler;
// Lock during creation and destruction of GUI
	Mutex *plugin_gui_lock;
// Lock during creation and destruction of brender so playback doesn't use it.
	Mutex *brender_lock;

	void init_render();
	void init_exportedl();
// These three happen synchronously with each other
// Make sure this is called after synchronizing EDL's.
	void init_brender();
// Delete brender
	void delete_brender();
// Restart brender after testing its existence
	void restart_brender();
// Stops brender after testing its existence
	void stop_brender();
// This one happens asynchronously of the others.  Used by playback to
// see what frame is background rendered.
	int brender_available(ptstime position);
	void set_brender_start();

	void init_error();
	static void init_defaults(BC_Hash* &defaults, 
		const char *config_path);
	const char *default_std();
	void init_edl();
	void init_awindow();
	void init_gwindow();
	void init_tipwindow();
// Used by MWindow and RenderFarmClient
	static void init_plugins(Preferences *preferences, 
		ArrayList<PluginServer*>* &plugindb,
		SplashGUI *splash_window);
	static void init_plugin_path(Preferences *preferences, 
		ArrayList<PluginServer*>* &plugindb,
		FileSystem *fs,
		SplashGUI *splash_window,
		int *counter);
	void init_preferences();
	void init_signals();
	void init_theme();
	void init_compositor();
	void init_levelwindow();
	void init_viewer();
	void init_ruler();
	void init_indexes();
	void init_gui();
	void init_playbackcursor();

	void clean_indexes();
	SigHandler *sighandler;
	ClipEdit *clip_edit;
private:
	time_t last_backup_time;
};

#endif
