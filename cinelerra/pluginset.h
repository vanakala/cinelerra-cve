
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

#ifndef PLUGINSET_H
#define PLUGINSET_H

#include <stdint.h>

#include "edits.h"
#include "edl.inc"
#include "keyframe.inc"
#include "module.inc"
#include "plugin.inc"
#include "pluginautos.inc"
#include "sharedlocation.inc"
#include "track.inc"

class PluginSet : public Edits
{
public:
	PluginSet(EDL *edl, Track *track);
	virtual ~PluginSet();

	virtual void synchronize_params(PluginSet *plugin_set);
	virtual PluginSet& operator=(PluginSet& plugins);
	virtual Plugin* create_plugin() { return 0; };
// Returns the point to restart background rendering at.
// -1 means nothing changed.
	void clear_keyframes(int64_t start, int64_t end);
// Clear edits only for a handle modification
	void clear_recursive(int64_t start, int64_t end);
	void shift_keyframes_recursive(int64_t position, int64_t length);
	void shift_effects_recursive(int64_t position, int64_t length);
	void clear(int64_t start, int64_t end);
	void copy_from(PluginSet *src);
	void copy(int64_t start, int64_t end, FileXML *file);
	void copy_keyframes(int64_t start, 
		int64_t end, 
		FileXML *file, 
		int default_only,
		int autos_only);
	static void paste_keyframes(int64_t start, 
		int64_t length, 
		FileXML *file, 
		int default_only,
		Track *track);
// Return the nearest boundary of any kind in the plugin edits
	int64_t plugin_change_duration(int64_t input_position, 
		int64_t input_length, 
		int reverse);
	void shift_effects(int64_t start, int64_t length);
	Edit* insert_edit_after(Edit *previous_edit);
	Edit* create_edit();
// For testing output equivalency when a new pluginset is added.
	Plugin* get_first_plugin();
// The plugin set number in the track
	int get_number();
	void save(FileXML *file);
	void load(FileXML *file, uint32_t load_flags);
	void dump();
	int optimize();

// Insert a new plugin
	Plugin* insert_plugin(char *title, 
		int64_t unit_position, 
		int64_t unit_length,
		int plugin_type,
		SharedLocation *shared_location,
		KeyFrame *default_keyframe,
		int do_optimize);

	PluginAutos *automation;
	int record;
};


#endif
