
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
	void clear_keyframes(ptstime start, ptstime end);
	void clear(ptstime start, ptstime end);
	void clear_after(ptstime pts);
	void copy_from(PluginSet *src);
	void save_xml(FileXML *file);
	void copy(PluginSet *src, ptstime start, ptstime end);
	Plugin *active_in(ptstime start, ptstime end);

	static void paste_keyframes(ptstime start,
		ptstime length,
		FileXML *file,
		Track *track);

// Return the nearest boundary of any kind in the plugin edits
	ptstime plugin_change_duration(ptstime input_position,
		ptstime input_length);
	void shift_effects(ptstime start, ptstime length);
	Edit* create_edit();
// For testing output equivalency when a new pluginset is added.
	Plugin* get_first_plugin();
// The plugin set number in the track
	int get_number();

	void load(FileXML *file, uint32_t load_flags);
	void dump(int indent = 0);

// Insert a new plugin
	Plugin* insert_plugin(const char *title, 
		ptstime position,
		ptstime length,
		int plugin_type,
		SharedLocation *shared_location);
};

#endif
