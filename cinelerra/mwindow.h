
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

#ifndef MWINDOW_H
#define MWINDOW_H

#include "arraylist.h"
#include "asset.inc"
#include "assets.inc"
#include "audiodevice.inc"
#include "awindow.inc"
#include "batchrender.inc"
#include "bcwindowbase.inc"
#include "brender.inc"
#include "cache.inc"
#include "channel.inc"
#include "channeldb.inc"
#include "cwindow.inc"
#include "bchash.inc"
#include "devicedvbinput.inc"
#include "edit.inc"
#include "edl.inc"
#include "exportedl.inc"
#include "filesystem.inc"
#include "filexml.inc"
#include "framecache.inc"
#include "gwindow.inc"
#include "levelwindow.inc"
#include "loadmode.inc"
#include "mainerror.inc"
#include "mainindexes.inc"
#include "mainprogress.inc"
#include "mainsession.inc"
#include "mainundo.inc"
#include "maxchannels.h"
#include "mutex.inc"
#include "mwindow.inc"
#include "mwindowgui.inc"
#include "new.inc"
#include "patchbay.inc"
#include "playback3d.inc"
#include "playbackengine.inc"
#include "plugin.inc"
#include "pluginserver.inc"
#include "pluginset.inc"
#include "preferences.inc"
#include "preferencesthread.inc"
#include "recordlabel.inc"
#include "removethread.inc"
#include "render.inc"
#include "sharedlocation.inc"
#include "sighandler.inc"
#include "splashgui.inc"
#include "theme.inc"
#include "thread.h"
#include "threadloader.inc"
#include "timebar.inc"
#include "timebomb.h"
#include "tipwindow.inc"
#include "track.inc"
#include "tracking.inc"
#include "tracks.inc"
#include "transition.inc"
#include "transportque.inc"
#include "videowindow.inc"
#include "vwindow.inc"
#include "wavecache.inc"

#include <stdint.h>

// All entry points for commands except for window locking should be here.
// This allows scriptability.

class MWindow : public Thread
{
public:
	MWindow();
	~MWindow();

// ======================================== initialization commands
	void create_objects(int want_gui, 
		int want_new,
		char *config_path);
	void show_splash();
	void hide_splash();
	void start();
	void run();

	int run_script(FileXML *script);
	int new_project();
	int delete_project(int flash = 1);

	int load_defaults();
	int save_defaults();
	int set_filename(char *filename);
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
	void tile_windows();
	void set_titles(int value);
	int asset_to_edl(EDL *new_edl, Asset *new_asset, RecordLabels *labels = 0);

// Entry point to insert assets and insert edls.  Called by TrackCanvas 
// and AssetPopup when assets are dragged in from AWindow.
// Takes the drag vectors from MainSession and
// pastes either assets or clips depending on which is full.
// Returns 1 if the vectors were full
	int paste_assets(double position, Track *dest_track, int overwrite);
	
// Insert the assets at a point in the EDL.  Called by menueffects,
// render, and CWindow drop but recording calls paste_edls directly for
// labels.
	void load_assets(ArrayList<Asset*> *new_assets, 
		double position, 
		int load_mode,
		Track *first_track /* = 0 */,
		RecordLabels *labels /* = 0 */,
		int edit_labels,
		int edit_plugins,
		int overwrite);
	int paste_edls(ArrayList<EDL*> *new_edls, 
		int load_mode, 
		Track *first_track /* = 0 */,
		double current_position /* = -1 */,
		int edit_labels,
		int edit_plugins,
		int overwrite);
// Reset everything for a load
	void update_project(int load_mode);
// Fit selected time to horizontal display range
	void fit_selection();
// Fit selected autos to the vertical display range
	void fit_autos(int doall);
	void change_currentautorange(int autogrouptype, int increment, int changemax);
	void expand_autos(int changeall, int domin, int domax);
	void shrink_autos(int changeall, int domin, int domax);
// move the window to include the cursor
	void find_cursor();
// Append a plugindb with pointers to the master plugindb
	void create_plugindb(int do_audio, 
		int do_video, 
		int is_realtime, 
		int is_transition,
		int is_theme,
		ArrayList<PluginServer*> &plugindb);
// Find the plugin whose title matches title and return it
	PluginServer* scan_plugindb(char *title,
		int data_type);
	void dump_plugins();



	
	int load_filenames(ArrayList<char*> *filenames, 
		int load_mode = LOAD_REPLACE,
// Cause the project filename on the top of the window to be updated.
// Not wanted for loading backups.
		int update_filename = 1,
		char *reel_name = "cin0000",
		int reel_number = 0,
		int overwrite_reel = 0);
	

// Print out plugins which are referenced in the EDL but not loaded.
	void test_plugins(EDL *new_edl, char *path);

	int interrupt_indexes();  // Stop index building


	int redraw_time_dependancies();     // after reconfiguring the time format, sample rate, frame rate

// =========================================== movement

	void next_time_format();
	void prev_time_format();
	void time_format_common();
	int reposition_timebar(int new_pixel, int new_height);
	int expand_sample(double fixed_sample = -1);    // fixed_sample is the sample that should hold fixed position on the screen after zooming, -1 = selection
	int zoom_in_sample(double fixed_sample = -1);
	int zoom_sample(int64_t zoom_sample, int64_t view_start = -1); // what's the supposed view start
	void zoom_amp(int64_t zoom_amp);
	void zoom_track(int64_t zoom_track);
	int fit_sample();
	int move_left(int64_t distance = 0);
	int move_right(int64_t distance = 0);
	void move_up(int64_t distance = 0);
	void move_down(int64_t distance = 0);

// seek to labels
// shift_down must be passed by the caller because different windows call
// into this
	int next_label(int shift_down);   
	int prev_label(int shift_down);
// seek to edit handles
	int next_edit_handle(int shift_down);
	int prev_edit_handle(int shift_down);  
	void trackmovement(int track_start);
	int samplemovement(int64_t view_start);     // view_start is pixels
	void select_all();
	int goto_start();
	int goto_end();
	int expand_y();
	int zoom_in_y();
	int expand_t();
	int zoom_in_t();
	void crop_video();
	void update_plugins();
// Call after every edit operation
	void save_backup();
	void show_plugin(Plugin *plugin);
	void hide_plugin(Plugin *plugin, int lock);
	void hide_plugins();
// Update plugins with configuration changes.
// Called by TrackCanvas::cursor_motion_event.
	void update_plugin_guis();
	void update_plugin_states();
	void update_plugin_titles();
// Called by Attachmentpoint during playback.
// Searches for matching plugin and renders data in it.
	void render_plugin_gui(void *data, Plugin *plugin);
	void render_plugin_gui(void *data, int size, Plugin *plugin);

// Called from PluginVClient::process_buffer
// Returns 1 if a GUI for the plugin is open so OpenGL routines can determine if
// they can run.
	int plugin_gui_open(Plugin *plugin);


// ============================= editing commands ========================

// Map each recordable audio track to the desired pattern
	void map_audio(int pattern);
	enum
	{
		AUDIO_5_1_TO_2,
		AUDIO_1_TO_1
	};
	void add_audio_track_entry(int above, Track *dst);
	int add_audio_track(int above, Track *dst);
	void add_clip_to_edl(EDL *edl);
	void add_video_track_entry(Track *dst = 0);
	int add_video_track(int above, Track *dst);

	void asset_to_size();
	void asset_to_rate();
// Entry point for clear operations.
	void clear_entry();
// Clears active region in EDL.
// If clear_handle, edit boundaries are cleared if the range is 0.
// Called by paste, record, menueffects, render, and CWindow drop.
	void clear(int clear_handle);
	void clear_labels();
	int clear_labels(double start, double end);
	void concatenate_tracks();
	void copy();
	int copy(double start, double end);
	void cut();

// Calculate aspect ratio from pixel counts
	static int create_aspect_ratio(float &w, float &h, int width, int height);
// Calculate defaults path
	static void create_defaults_path(char *string);

	void delete_folder(char *folder);
	void delete_inpoint();
	void delete_outpoint();    

	void delete_track();
	void delete_track(Track *track);
	void delete_tracks();
	void detach_transition(Transition *transition);
	int feather_edits(int64_t feather_samples, int audio, int video);
	int64_t get_feather(int audio, int video);
	float get_aspect_ratio();
	void insert(double position, 
		FileXML *file,
		int edit_labels,
		int edit_plugins,
		EDL *parent_edl = 0);

// TrackCanvas calls this to insert multiple effects from the drag_pluginservers
// into pluginset_highlighted.
	void insert_effects_canvas(double start,
		double length);

// CWindow calls this to insert multiple effects from 
// the drag_pluginservers array.
	void insert_effects_cwindow(Track *dest_track);

// This is called multiple times by the above functions.
// It can't sync parameters.
	void insert_effect(char *title, 
		SharedLocation *shared_location, 
		Track *track,
		PluginSet *plugin_set,
		double start,
		double length,
		int plugin_type);

	void match_output_size(Track *track);
// Move edit to new position
	void move_edits(ArrayList<Edit*> *edits,
		Track *track,
		double position,
		int behaviour);       // behaviour: 0 - old style (cut and insert elswhere), 1- new style - (clear and overwrite elsewere)
// Move effect to position
	void move_effect(Plugin *plugin,
		PluginSet *plugin_set,
		Track *track,
		int64_t position);
	void move_plugins_up(PluginSet *plugin_set);
	void move_plugins_down(PluginSet *plugin_set);
	void move_track_down(Track *track);
	void move_tracks_down();
	void move_track_up(Track *track);
	void move_tracks_up();
	void mute_selection();
	void new_folder(char *new_folder);
	void overwrite(EDL *source);
// For clipboard commands
	void paste();
// For splice and overwrite
	int paste(double start, 
		double end, 
		FileXML *file,
		int edit_labels,
		int edit_plugins);
	int paste_output(int64_t startproject, 
				int64_t endproject, 
				int64_t startsource_sample, 
				int64_t endsource_sample, 
				int64_t startsource_frame,
				int64_t endsource_frame,
				Asset *asset, 
				RecordLabels *new_labels);
	void paste_silence();

	void paste_transition();
	void paste_transition_cwindow(Track *dest_track);
	void paste_audio_transition();
	void paste_video_transition();
	void rebuild_indices();
// Asset removal from caches
	void reset_caches();
	void remove_asset_from_caches(Asset *asset);
	void remove_assets_from_project(int push_undo = 0);
	void remove_assets_from_disk();
	void resize_track(Track *track, int w, int h);
	void set_auto_keyframes(int value);
	void set_labels_follow_edits(int value);

// Update the editing mode
	int set_editing_mode(int new_editing_mode);
	void toggle_editing_mode();
	void set_inpoint(int is_mwindow);
	void set_outpoint(int is_mwindow);
	void splice(EDL *source);
	void toggle_loop_playback();
	void trim_selection();
// Synchronize EDL settings with all playback engines depending on current 
// operation.  Doesn't redraw anything.
	void sync_parameters(int change_type = CHANGE_PARAMS);
	void to_clip();
	int toggle_label(int is_mwindow);
	void undo_entry(BC_WindowBase *calling_window_gui);
	void redo_entry(BC_WindowBase *calling_window_gui);


	int cut_automation();
	int copy_automation();
	int paste_automation();
	void clear_automation();
	void straighten_automation();
	int cut_default_keyframe();
	int copy_default_keyframe();
// Use paste_automation to paste the default keyframe in other position.
// Use paste_default_keyframe to replace the default keyframe with whatever is
// in the clipboard.
	int paste_default_keyframe();
	int clear_default_keyframe();

	int modify_edithandles();
	int modify_pluginhandles();
	void finish_modify_handles();

	
	
	
	
	

// Send new EDL to caches
	void age_caches();
	int optimize_assets();            // delete unused assets from the cache and assets


	void select_point(double position);
	int set_loop_boundaries();         // toggle loop playback and set boundaries for loop playback


	Playback3D *playback_3d;
	RemoveThread *remove_thread;
	
	SplashGUI *splash_window;
// Main undo stack
	MainUndo *undo;
	BC_Hash *defaults;
	Assets *assets;
// CICaches for drawing timeline only
	CICache *audio_cache, *video_cache;
// Frame cache for drawing timeline only.
// Cache drawing doesn't wait for file decoding.
	FrameCache *frame_cache;
	WaveCache *wave_cache;
	Preferences *preferences;
	PreferencesThread *preferences_thread;
	MainSession *session;
	Theme *theme;
	MainIndexes *mainindexes;
	MainProgress *mainprogress;
	BRender *brender;

// Menu items
	ArrayList<ColormodelItem*> colormodels;
	ArrayList<InterlaceautofixoptionItem*> interlace_asset_autofixoptions;
	ArrayList<InterlacemodeItem*>          interlace_project_modes;
	ArrayList<InterlacemodeItem*>          interlace_asset_modes;
	ArrayList<InterlacefixmethodItem*>     interlace_asset_fixmethods;

	int reset_meters();

// Channel DB for playback only.  Record channel DB's are in record.C
	ChannelDB *channeldb_buz;
	ChannelDB *channeldb_v4l2jpeg;

// ====================================== plugins ==============================

// Contain file descriptors for all the dlopens
	ArrayList<PluginServer*> *plugindb;
// Currently visible plugins
	ArrayList<PluginServer*> *plugin_guis;


// Adjust sample position to line up with frames.
	int fix_timing(int64_t &samples_out, 
		int64_t &frames_out, 
		int64_t samples_in);


	BatchRenderThread *batch_render;
	Render *render;

 	ExportEDL *exportedl;


// Master edl
	EDL *edl;
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
// Lock during creation and destruction of GUI
	Mutex *plugin_gui_lock;
// Lock during creation and destruction of brender so playback doesn't use it.
	Mutex *brender_lock;

// Single device drivers which must be shared between audio and video go here.
// They are managed by the garbage collector.
	DeviceDVBInput *dvb_input;
// Must be locked before accessing dvb_input or Garbage functions in it.
	Mutex *dvb_input_lock;


// Initialize shared memory
	void init_shm();

// Initialize channel DB's for playback
	void init_channeldb();
	void init_render();
	void init_exportedl();
// These three happen synchronously with each other
// Make sure this is called after synchronizing EDL's.
	void init_brender();
// Restart brender after testing its existence
	void restart_brender();
// Stops brender after testing its existence
	void stop_brender();
// This one happens asynchronously of the others.  Used by playback to
// see what frame is background rendered.
	int brender_available(int position);
	void set_brender_start();

	void init_error();
	static void init_defaults(BC_Hash* &defaults, 
		char *config_path);
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
	void init_cache();
	void init_menus();
	void init_indexes();
	void init_gui();
	void init_3d();
	void init_playbackcursor();
	void delete_plugins();
// 
	void clean_indexes();
//	TimeBomb timebomb;
	SigHandler *sighandler;
};

#endif
