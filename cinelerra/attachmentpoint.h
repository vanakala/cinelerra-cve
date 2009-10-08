
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

#ifndef ATTACHMENTPOINT_H
#define ATTACHMENTPOINT_H

#include "arraylist.h"
#include "filexml.inc"
#include "floatauto.inc"
#include "floatautos.inc"
#include "mwindow.inc"
#include "messages.inc"
#include "plugin.inc"
#include "pluginserver.inc"
#include "renderengine.inc"
#include "sharedlocation.h"
#include "vframe.inc"
#include "virtualnode.inc"

#include <stdint.h>

// Attachment points for Modules to attach plugins
class AttachmentPoint
{
public:
	AttachmentPoint(RenderEngine *renderengine, 
		Plugin *plugin, 
		int data_type);
	virtual ~AttachmentPoint();

	virtual int reset_parameters();
// Used by Module::swap_attachments before virtual console expansion.
// Return 1 if the plugin id is the same
	int identical(AttachmentPoint *old);

// Move new_virtual_plugins to virtual_plugins.
// Called after virtual console expansion.
	int render_init();

// Cause plugins to free any resources which are required after stopping
	void render_stop();

// Called before every buffer processing
	void reset_status();

// Attach a virtual plugin for realtime playback.  Returns the number
// of the buffer assigned to a multichannel plugin.
	int attach_virtual_plugin(VirtualNode *virtual_plugin);
	virtual void delete_buffer_vector() {};

// return 0 if ready to render
// check all the virtual plugins for waiting status
// all virtual plugins attached to this must be waiting for a render
//	int sort(VirtualNode *virtual_plugin);
// Called by plugin server to render GUI with data.
	void render_gui(void *data);
	void render_gui(void *data, int size);
	int gui_open();
	virtual int get_buffer_size() { return 0; };

// For unshared plugins, virtual plugins to send configuration events to and 
// read data from.
// For shared plugins, virtual plugins to allocate buffers for and read 
// data from.
	ArrayList<VirtualNode*> virtual_plugins;

// List for a new virtual console which is transferred to virtual_plugins after
// virtual console expansion.
	ArrayList<VirtualNode*> new_virtual_plugins;

// Plugin servers to do signal processing
	ArrayList<PluginServer*> plugin_servers;

// renderengine Plugindb entry
	PluginServer *plugin_server;
// Pointer to the plugin object in EDL
	Plugin *plugin;
// ID of plugin object in EDL
	int plugin_id;
	RenderEngine *renderengine;
// Current input buffer being loaded.  Determines when the plugin server is run
//	int current_buffer;

// Status of last buffer processed.
// Used in shared multichannel plugin to tell of it's already been processed
// or needs to be processed again for a different target.
// start_position is the end of the range if playing in reverse.
	int64_t start_position;
	int64_t len;
	int64_t sample_rate;
	double frame_rate;
	int is_processed;
	int data_type;	











	int multichannel_shared(int search_new);
	int singlechannel();



	int dump();

};

#endif
