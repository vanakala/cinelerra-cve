
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

#ifndef MAINSESSION_H
#define MAINSESSION_H

#include "asset.inc"
#include "assets.inc"
#include "auto.inc"
#include "bchash.inc"
#include "edit.inc"
#include "edits.inc"
#include "edl.inc"
#include "guicast.h"
#include "mainsession.inc"
#include "maxchannels.h"
#include "mwindow.inc"
#include "plugin.inc"
#include "pluginset.inc"
#include "pluginserver.inc"
#include "track.inc"

// Options not in EDL but not changed in preferences
class MainSession
{
public:
	MainSession(MWindow *mwindow);
	~MainSession();

	int load_defaults(BC_Hash *defaults);
	int save_defaults(BC_Hash *defaults);
	void default_window_positions();
	void boundaries();





// For drag and drop events

// The entire track where the dropped asset is going to go
	Track *track_highlighted;
// The edit after the point where the media is going to be dropped.
	Edit *edit_highlighted;
// The plugin set where the plugin is going to be dropped.
	PluginSet *pluginset_highlighted;
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
// When trim should only affect the selected edits or plugins
	Edits *trim_edits;
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
	long drag_start_position;
// Records for redrawing brender position in timebar
	double brender_end;

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
// record monitor
	int rmonitor_x, rmonitor_y, rmonitor_w, rmonitor_h;
// record status
	int rwindow_x, rwindow_y, rwindow_w, rwindow_h;
// error window
	int ewindow_w, ewindow_h;
	int afolders_w;
	int show_vwindow, show_awindow, show_cwindow, show_gwindow, show_lwindow;
	int plugindialog_w, plugindialog_h;
	int menueffect_w, menueffect_h;

	int cwindow_fullscreen;
	int rwindow_fullscreen;
	int vwindow_fullscreen;

// Tip of the day
	int current_tip;

	MWindow *mwindow;
};

#endif
