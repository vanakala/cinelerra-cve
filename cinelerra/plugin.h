
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

#ifndef PLUGIN_H
#define PLUGIN_H

#include "guicast.h"
#include "edit.h"
#include "edl.inc"
#include "filexml.inc"
#include "keyframe.inc"
#include "keyframes.inc"
#include "module.inc"
#include "plugin.inc"
#include "pluginset.inc"
#include "pluginpopup.inc"
#include "pluginserver.inc"
#include "renderengine.inc"
#include "sharedlocation.h"
#include "virtualnode.inc"

class PluginOnToggle;



// Plugin is inherited by Transition, Plugins
class Plugin : public Edit
{
public:
// Plugin which belongs to a transition.
	Plugin(EDL *edl, 
		Track *track, 
		char *title);
// Called by  PluginSet::create_edit, PluginSet::insert_edit_after.
// Plugin can't take a track because it would get the edits pointer from 
// the track instead of the plugin set.
	Plugin(EDL *edl, 
		PluginSet *plugin_set, 
		char *title);
	virtual ~Plugin();

	virtual Plugin& operator=(Plugin& edit);
	virtual Edit& operator=(Edit& edit);

// Called by Edits::equivalent_output to override the keyframe behavior and check
// title.
	void equivalent_output(Edit *edit, int64_t *result);

// Called by playable tracks to test for playable server.
// Descends the plugin tree without creating a virtual console.
	int is_synthesis(RenderEngine *renderengine, 
		int64_t position, 
		int direction);

	virtual int operator==(Plugin& that);
	virtual int operator==(Edit& that);

	virtual void copy_from(Edit *edit);


// Called by == operators, Edit::equivalent output
// to test title and keyframe of transition.
	virtual int identical(Plugin *that);
// Called by render_gui.  Only need the track, position, and pluginset
// to determine a corresponding GUI.
	int identical_location(Plugin *that);
	virtual void synchronize_params(Edit *edit);
// Used by Edits::insert_edits and Plugin::shift to shift plugin keyframes
	void shift_keyframes(int64_t position);

	void change_plugin(char *title, 
		SharedLocation *shared_location, 
		int plugin_type);
// For synchronizing parameters
	void copy_keyframes(Plugin *plugin);
// For copying to clipboard
	void copy_keyframes(int64_t start, 
		int64_t end, 
		FileXML *file, 
		int default_only,
		int autos_only);
// For editing automation.  
// Returns the point to restart background rendering at.
// -1 means nothing changed.
	void clear_keyframes(int64_t start, int64_t end);
	void copy(int64_t start, int64_t end, FileXML *file);
	void paste(FileXML *file);
	void load(FileXML *file);
// Shift in time
	void shift(int64_t difference);
	void dump();
// Called by PluginClient sequence to get rendering parameters
	KeyFrame* get_prev_keyframe(int64_t position, int direction);
	KeyFrame* get_next_keyframe(int64_t position, int direction);
// If this is a standalone plugin fill its location in the result.
// If it's shared copy the shared location into the result
	void get_shared_location(SharedLocation *result);
// Get keyframes for editing with automatic creation if enabled.
// The direction is always assumed to be forward.
	virtual KeyFrame* get_keyframe();
	int silence();
// Calculate title given plugin type.  Used by TrackCanvas::draw_plugins
	void calculate_title(char *string, int use_nudge);
// Resolve objects pointed to by shared_location
	Track* get_shared_track();
//	Plugin* get_shared_plugin();

// Need to resample keyframes
	void resample(double old_rate, double new_rate);

// The title of the plugin is stored and not the plugindb entry in case it doesn't exist in the db
// Title of the plugin currently attached
	char title[BCTEXTLEN];           
	int plugin_type;
// In and out aren't used anymore.
	int in, out;
	int show, on;
	PluginSet *plugin_set;

// Data for the plugin is stored here.  Default keyframe always exists.
// As for storing in PluginSet instead of Plugin:

// Each plugin needs a default keyframe of its own.
// The keyframes are meaningless except for the plugin they're stored in.
// Default keyframe has position = 0.
// Other keyframes have absolute position.
	KeyFrames *keyframes;

// location of plugin if shared
	SharedLocation shared_location;
};






#endif
