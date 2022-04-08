// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef EDL_H
#define EDL_H

#include "asset.inc"
#include "datatype.h"
#include "bcwindowbase.inc"
#include "arraylist.h"
#include "autoconf.inc"
#include "bchash.inc"
#include "edits.inc"
#include "edl.inc"
#include "edlsession.inc"
#include "filexml.inc"
#include "labels.inc"
#include "localsession.inc"
#include "mutex.inc"
#include "plugin.inc"
#include "preferences.inc"
#include "theme.inc"
#include "track.inc"
#include "tracks.inc"


// Loading and saving are built on load and copy except for automation:

// Storage: 
// Load: load new -> paste into master
// Save: copy all of master
// Undo: selective load into master
// Copy: copy from master
// Paste: load new -> paste into master
// Copy automation: copy just automation from master
// Paste automation: paste functions in automation

class EDL
{
public:
	EDL(int is_master);
	~EDL();

	void reset_instance();
	void reset_plugins();
	void reset_renderers();

	EDL& operator=(EDL &edl);

// Load configuration and track counts
	void load_defaults(BC_Hash *defaults, EDLSession *session);
	void save_defaults(BC_Hash *defaults, EDLSession *session);
// Clip default settings to boundaries.
	void boundaries();
// Create tracks using existing configuration
	void create_default_tracks();
	void load_xml(FileXML *file, EDLSession *session);
	void save_xml(FileXML *xml, const char *output_path = 0,
		int save_flags = 0);

// Convert position to frames if cursor alignment is enabled
	ptstime align_to_frame(ptstime position, int round = 1);

// Plugin and client
	void update_plugin_guis();
	void update_plugin_titles();

// Scale all sample values since everything is locked to audio
	void rechannel();
	void copy_tracks(EDL *edl);

// Copies project path, folders, and LocalSession from edl argument.
// Sets this_edlsession to session if given
	void copy_session(EDL *edl, EDLSession *session = 0);
	void copy_all(EDL *edl);

// Determine if the positions are equivalent if they're within half a frame
// of each other.
	int equivalent(double position1, double position2);

// Determine if the EDL's produce equivalent video output to the old EDL.
// The new EDL is this and the old EDL is the argument.
// Return the number of seconds from the beginning of this which are 
// equivalent to the argument.
// If they're completely equivalent, -1 is returned;
// This is used by BRender.
	double equivalent_output(EDL *edl);

// Set project path for saving a backup
	void set_project_path(const char *path);

// Set points and labels
	void set_inpoint(ptstime position);
	void set_outpoint(ptstime position);

// Add assets from the src to the destination
	void update_assets(EDL *src);

	void update_assets(Asset *asset);
	void optimize();

// Debug
	void dump(int indent = 0);
	void dump_assets(int indent = 0);

	static int next_id();

	ptstime adjust_position(ptstime oldposition,
		ptstime newposition,
		int currentend,
		int handle_mode);

	void modify_edithandles(ptstime oldposition,
		ptstime newposition,
		int currentend, 
		int handle_mode,
		int edit_labels);

	void modify_pluginhandles(ptstime oldposition,
		ptstime newposition,
		int currentend, 
		int handle_mode);

	void trim_selection(ptstime start, 
		ptstime end,
		int edit_labels);

// Editing functions
	void copy_assets(ptstime start,
		ptstime end,
		FileXML *file, 
		int all, 
		const char *output_path);
	void copy(EDL *edl, ptstime start, ptstime end,
		ArrayList<Track*> *src_tracks = 0);
	void paste_silence(ptstime start,
		ptstime end,
		int edit_labels);
	void remove_from_project(ArrayList<Asset*> *assets);
	void clear(ptstime start, ptstime end, Track *first_track = 0);
// Insert the asset at a point in the EDL
	void insert_asset(Asset *asset, 
		double position, 
		Track *first_track = 0);
// Insert the clip at a point in the EDL
	int insert_clips(ArrayList<EDL*> *new_edls, int load_mode, Track *first_track = 0);

	void get_shared_plugins(Track *source, ptstime startpos, ptstime endpos,
		ArrayList<Plugin*> *plugin_locations);
	void get_shared_tracks(Track *track, ptstime start, ptstime end,
		ArrayList<Track*> *module_locations);

	int get_tracks_height(Theme *theme);

// Return dimensions for canvas if smaller dimensions has zoom of 1
	void calculate_conformed_dimensions(double &w, double &h);
// Get the total output size scaled to aspect ratio
	void output_dimensions_scaled(int &w, int &h);
	double get_sample_aspect_ratio();
// Create tracks from assets
	void init_edl();

	ptstime duration();
	ptstime duration_of(int type);
	int total_tracks();
	int total_tracks_of(int type);
	int playable_tracks_of(int type);
	int recordable_tracks_of(int type);
// Pointer to the first track
	Track *first_track();
// Number of pointed track
	int number_of(Track *track);
// Pointer to track number
	Track *number(int number);
// Set toggles of all tracks to value
	void set_all_toggles(int toggle_type, int value);
// Count number of toggels set
	int total_toggled(int toggle_type);
// Ensure that only one track is master
	void check_master_track();
// Name of the handle
	static const char *handle_name(int handle);
// Size of allocated memory
	size_t get_size();

// Media files
// Shared between all EDLs
	ArrayList<Asset*> *assets;

	Tracks *tracks;
	Labels *labels;

// Specific to this EDL, for clips.
	LocalSession *local_session;
// EDLSession specific of this edl
// Not owned by this edl
	EDLSession *this_edlsession;

// In the top EDL, this is the path it was loaded from.  Restores 
// project titles from backups.  This is only used for loading backups.
// All other loads keep the path in mainsession->filename.
// This can't use the output_path argument to save_xml because that points
// to the backup file, not the project file.
	char project_path[BCTEXTLEN];

	static Mutex *id_lock;

// unique ID of this EDL for resource window
	int id;
	int is_master;
};

#endif
