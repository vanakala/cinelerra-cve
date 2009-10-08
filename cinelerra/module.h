
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

#ifndef MODULE_H
#define MODULE_H

#include "attachmentpoint.inc"
#include "cache.inc"
#include "commonrender.inc"
#include "datatype.h"
#include "edl.inc"
#include "filexml.inc"
#include "guicast.h"
#include "maxchannels.h"
#include "module.inc"
#include "patch.inc"
#include "plugin.inc"
#include "pluginarray.inc"
#include "pluginserver.inc"
#include "pluginset.inc"
#include "renderengine.inc"
#include "sharedlocation.inc"
#include "track.inc"

class Module
{
public:
	Module(RenderEngine *renderengine, 
		CommonRender *commonrender, 
		PluginArray *plugin_array,
		Track *track);
	Module() {};
	virtual ~Module();

	virtual void create_objects();
	void create_new_attachments();
// Swaps in changed plugin servers for old plugins servers during playback.
// Allows data in unchanged plugins to continue.  Prepares pointers in
// plugin server to be added in expansion.
	void swap_attachments();
// Reset processing status of attachments before every buffer is processed.
	void reset_attachments();
	virtual AttachmentPoint* new_attachment(Plugin *plugin) { return 0; };
	virtual int get_buffer_size() { return 0; };
	int test_plugins();
	AttachmentPoint* attachment_of(Plugin *plugin);
	
// Get attachment number or return 0 if out of range.
	AttachmentPoint* get_attachment(int number);

	void dump();
// Start plugin rendering
	int render_init();
// Stop plugin rendering in case any resources have to be freed.
	void render_stop();
// Current_position is relative to the EDL rate.
// If direction is REVERSE, the object before current_position is tested.
	void update_transition(int64_t current_position, int direction);
	EDL* get_edl();

// CICache used during effect
	CICache *cache;
// EDL used during effect
	EDL *edl;
// Not available in menu effects
	CommonRender *commonrender;
// Not available in menu effects
	RenderEngine *renderengine;
// Not available in realtime playback
	PluginArray *plugin_array;
	Track *track;
// TRACK_AUDIO or TRACK_VIDEO
	int data_type;       

// Pointer to transition in EDL
	Plugin *transition;
// PluginServer for transition
	PluginServer *transition_server;

// Currently active plugins.
// Use one AttachmentPoint for every pluginset to allow shared plugins to create
// extra plugin servers.
// AttachmentPoints are 0 if there is no plugin on the pluginset.
	AttachmentPoint **attachments;
	int total_attachments;
// AttachmentPoints are swapped in at render start to keep unchanged modules
// from resetting
	AttachmentPoint **new_attachments;
	int new_total_attachments;
};

#endif
