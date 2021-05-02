// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef MAINSESSION_H
#define MAINSESSION_H

#include "asset.inc"
#include "auto.inc"
#include "arraylist.h"
#include "bcwindowbase.inc"
#include "bchash.inc"
#include "datatype.h"
#include "edit.inc"
#include "edits.inc"
#include "edl.inc"
#include "mainsession.inc"
#include "plugin.inc"
#include "pluginserver.inc"
#include "track.inc"

// Options not in EDL but not changed in preferences
class MainSession
{
public:
	MainSession();
	~MainSession();

	void load_defaults(BC_Hash *defaults);
	void save_defaults(BC_Hash *defaults);
	void default_window_positions();
	void boundaries();
	size_t get_size();

// For drag and drop events

// The entire track where the dropped asset is going to go
	Track *track_highlighted;
// The edit after the point where the media is going to be dropped.
	Edit *edit_highlighted;
// The plugin after the point where the plugin is going to be dropped.
	Plugin *plugin_highlighted;
// Viewer canvas highlighted
	int vcanvas_highlighted;
// Compositor canvas highlighted
	int ccanvas_highlighted;
// Current drag operation
	int current_operation;
// Item being dragged
	ArrayList <PluginServer*> *drag_pluginservers;
	Plugin *drag_plugin;
	ArrayList<Asset*> *drag_assets;
	ArrayList<EDL*> *drag_clips;
	Auto *drag_auto;
	ArrayList<Auto*> *drag_auto_gang;

// Edit whose handle is being dragged
	Edit *drag_edit;
// Edits who are being dragged
	ArrayList<Edit*> *drag_edits;
// Button pressed during drag
	int drag_button;
// Handle being dragged
	int drag_handle;
// Current position of drag cursor
	double drag_position;
// Starting position of drag cursor
	double drag_start;
// Cursor position when button was pressed
	int drag_origin_x, drag_origin_y;
// Value of keyframe when button was pressed
	float drag_start_percentage;
	ptstime drag_start_postime;
// Records for redrawing brender position in timebar
	ptstime brender_end;
// Position of cursor in CWindow output.  Used by ruler.
	int cwindow_output_x, cwindow_output_y;

// Show controls in CWindow
	int cwindow_controls;

// Clip number for automatic title generation
	int clip_number;

// Audio session
	int changes_made;

// filename of the current project for window titling and saving
	char filename[BCTEXTLEN];

	int batchrender_x, batchrender_y, batchrender_w, batchrender_h;

// Window positions
// level window
	int lwindow_x, lwindow_y, lwindow_w, lwindow_h;
// main window
	int mwindow_x, mwindow_y, mwindow_w, mwindow_h;
// viewer
	int vwindow_x, vwindow_y, vwindow_w, vwindow_h;
// compositor
	int cwindow_x, cwindow_y, cwindow_w, cwindow_h;
	int ctool_x, ctool_y;
// asset window
	int awindow_x, awindow_y, awindow_w, awindow_h;
	int gwindow_x, gwindow_y;
// ruler
	int ruler_x, ruler_y, ruler_length, ruler_orientation;
// error window
	int ewindow_w, ewindow_h;
	int afolders_w;
	int show_vwindow, show_awindow, show_cwindow, show_gwindow, show_lwindow;
	int show_ruler;
	int plugindialog_w, plugindialog_h;
	int menueffect_w, menueffect_h;

	int cwindow_fullscreen;
	int rwindow_fullscreen;
	int vwindow_fullscreen;

// Tip of the day
	int current_tip;
};

#endif
