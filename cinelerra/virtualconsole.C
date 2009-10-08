
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

#include "auto.h"
#include "automation.h"
#include "autos.h"
#include "commonrender.h"
#include "condition.h"
#include "edl.h"
#include "edlsession.h"
#include "virtualconsole.h"
#include "module.h"
#include "mutex.h"
#include "playabletracks.h"
#include "renderengine.h"
#include "intautos.h"
#include "tracks.h"
#include "transportque.h"
#include "virtualnode.h"


VirtualConsole::VirtualConsole(RenderEngine *renderengine, 
	CommonRender *commonrender,
	int data_type)
{
	this->renderengine = renderengine;
	this->commonrender = commonrender;
	this->data_type = data_type;
	total_exit_nodes = 0;
	playable_tracks = 0;
	entry_nodes = 0;
	debug_tree = 0;
}


VirtualConsole::~VirtualConsole()
{
	delete_virtual_console();

	delete playable_tracks;
}


void VirtualConsole::create_objects()
{
	interrupt = 0;
	done = 0;

	get_playable_tracks();
	total_exit_nodes = playable_tracks->total;
	build_virtual_console(1);
//dump();
}

void VirtualConsole::start_playback()
{
	interrupt = 0;
	done = 0;
}

void VirtualConsole::get_playable_tracks()
{
}

Module* VirtualConsole::module_of(Track *track)
{
	for(int i = 0; i < commonrender->total_modules; i++)
	{
		if(commonrender->modules[i]->track == track) 
			return commonrender->modules[i];
	}
	return 0;
}

Module* VirtualConsole::module_number(int track_number)
{
// The track number is an absolute number of the track independant of
// the tracks with matching data type but virtual modules only exist for
// the matching data type.
// Convert from absolute track number to data type track number.
	Track *current = renderengine->edl->tracks->first;
	int data_type_number = 0, number = 0;

	for( ; current; current = NEXT, number++)
	{
		if(current->data_type == data_type)
		{
			if(number == track_number)
				return commonrender->modules[data_type_number];
			else
				data_type_number++;
		}
	}


	return 0;
}

void VirtualConsole::build_virtual_console(int persistent_plugins)
{
// allocate the entry nodes
	if(!entry_nodes)
	{
		entry_nodes = new VirtualNode*[total_exit_nodes];

		for(int i = 0; i < total_exit_nodes; i++)
		{
			entry_nodes[i] = new_entry_node(playable_tracks->values[i], 
				module_of(playable_tracks->values[i]), 
				i);

// Expand the trees
			entry_nodes[i]->expand(persistent_plugins, 
				commonrender->current_position);
		}
		commonrender->restart_plugins = 1;
	}
//dump();
}

VirtualNode* VirtualConsole::new_entry_node(Track *track, 
	Module *module, 
	int track_number)
{
	printf("VirtualConsole::new_entry_node should not be called\n");
	return 0;
}

void VirtualConsole::append_exit_node(VirtualNode *node)
{
	node->is_exit = 1;
	exit_nodes.append(node);
}

void VirtualConsole::reset_attachments()
{
	for(int i = 0; i < commonrender->total_modules; i++)
	{
		commonrender->modules[i]->reset_attachments();
	}
}

void VirtualConsole::dump()
{
	printf("VirtualConsole\n");
	printf(" Modules\n");
	for(int i = 0; i < commonrender->total_modules; i++)
		commonrender->modules[i]->dump();
	printf(" Nodes\n");
	for(int i = 0; i < total_exit_nodes; i++)
		entry_nodes[i]->dump(0);
}


int VirtualConsole::test_reconfigure(int64_t position, 
	int64_t &length, 
	int &last_playback)
{
	int result = 0;
	Track *current_track;
	Module *module;

// Test playback status against virtual console for current position.
	for(current_track = renderengine->edl->tracks->first;
		current_track && !result;
		current_track = current_track->next)
	{
		if(current_track->data_type == data_type)
		{
// Playable status changed
			if(playable_tracks->is_playable(current_track, 
				commonrender->current_position,
				1))
			{
				if(!playable_tracks->is_listed(current_track))
					result = 1;
			}
			else
			if(playable_tracks->is_listed(current_track))
			{
				result = 1;
			}
		}
	}

// Test plugins against virtual console at current position
	for(int i = 0; i < commonrender->total_modules && !result; i++)
		result = commonrender->modules[i]->test_plugins();





// Now get the length of time until next reconfiguration.
// This part is not concerned with result.
// Don't clip input length if only rendering 1 frame.
	if(length == 1) return result;





	int direction = renderengine->command->get_direction();
// GCC 3.2 requires this or optimization error results.
	int64_t longest_duration1;
	int64_t longest_duration2;
	int64_t longest_duration3;

// Length of time until next transition, edit, or effect change.
// Why do we need the edit change?  Probably for changing to and from silence.
	for(current_track = renderengine->edl->tracks->first;
		current_track;
		current_track = current_track->next)
	{
		if(current_track->data_type == data_type)
		{
// Test the transitions
			longest_duration1 = current_track->edit_change_duration(
				commonrender->current_position, 
				length, 
				direction == PLAY_REVERSE, 
				1,
				1);


// Test the edits
			longest_duration2 = current_track->edit_change_duration(
				commonrender->current_position, 
				length, 
				direction, 
				0,
				1);


// Test the plugins
			longest_duration3 = current_track->plugin_change_duration(
				commonrender->current_position,
				length,
				direction == PLAY_REVERSE,
				1);

			if(longest_duration1 < length)
			{
				length = longest_duration1;
				last_playback = 0;
			}
			if(longest_duration2 < length)
			{
				length = longest_duration2;
				last_playback = 0;
			}
			if(longest_duration3 < length)
			{
				length = longest_duration3;
				last_playback = 0;
			}

		}
	}

	return result;
}





int VirtualConsole::delete_virtual_console()
{
// delete the virtual node tree
	for(int i = 0; i < total_exit_nodes; i++)
	{
		delete entry_nodes[i];
	}
// Seems to get allocated even if new[0].
	if(entry_nodes) delete [] entry_nodes;
	entry_nodes = 0;
	exit_nodes.remove_all();
}


