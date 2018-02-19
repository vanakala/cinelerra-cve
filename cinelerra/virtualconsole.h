
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

#ifndef COMMONRENDERTHREAD_H
#define COMMONRENDERTHREAD_H

#include "arraylist.h"
#include "commonrender.inc"
#include "module.inc"
#include "playabletracks.inc"
#include "renderengine.inc"
#include "track.inc"
#include "virtualnode.inc"

// Virtual console runs synchronously for audio and video in
// pull mode.
class VirtualConsole
{
public:
	VirtualConsole(RenderEngine *renderengine, 
		CommonRender *commonrender, 
		int data_type);
	virtual ~VirtualConsole();

	void create_nodes();
	void start_playback();

// Called during each process buffer operation to reset the status
// of the attachments to unprocessed.
	void reset_attachments();
	void dump(int indent = 0);

// Create a new entry node in subclass of desired type.
// was new_toplevel_node
	virtual VirtualNode* new_entry_node(Track *track, 
		Module *module, 
		int track_number) { return 0; };
// Append exit node to table when expansion hits the end of a tree.
	void append_exit_node(VirtualNode *node);

	Module* module_of(Track *track);
	Module* module_number(int track_number);
// Test for reconfiguration.
// If reconfiguration is coming up, truncate length and reset last_playback.
	int test_reconfigure(ptstime &length,
		int &last_playback);

	RenderEngine *renderengine;
	CommonRender *commonrender;

// Total exit nodes.  Corresponds to the total playable tracks.
// Was total_tracks
	int total_exit_nodes;
// Current exit node being processed.  Used to speed up video.
	int current_exit_node;
// Entry node for each playable track
// Was toplevel_nodes
	VirtualNode **entry_nodes;

// Exit node for each playable track.  Rendering starts here and data is pulled
// up the tree.  Every virtual module is an exit node.
	ArrayList<VirtualNode*> exit_nodes;

	int data_type;

// exit conditions
	int interrupt;
	int done;

	PlayableTracks *playable_tracks;
};

#endif
