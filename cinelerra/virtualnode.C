
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

#include "attachmentpoint.h"
#include "auto.h"
#include "automation.h"
#include "autos.h"
#include "floatauto.h"
#include "floatautos.h"
#include "intauto.h"
#include "intautos.h"
#include "mwindow.h"
#include "module.h"
#include "panauto.h"
#include "panautos.h"
#include "patchbay.h"
#include "plugin.h"
#include "pluginserver.h"
#include "renderengine.h"
#include "tracks.h"
#include "transition.h"
#include "transportque.h"
#include "virtualconsole.h"
#include "virtualnode.h"

VirtualNode::VirtualNode(RenderEngine *renderengine, 
		VirtualConsole *vconsole, 
		Module *real_module, 
		Plugin *real_plugin,
		Track *track, 
		VirtualNode *parent_node)
{
	this->renderengine = renderengine;
	this->vconsole = vconsole;
	this->real_module = real_module;
	this->real_plugin = real_plugin;
	this->track = track;
	this->parent_node = parent_node;
	render_count = 0;
	plugin_type = 0;
	waiting_real_plugin = 0;
	plugin_buffer_number = 0;
	plugin_autos = 0;
	plugin_auto_before = plugin_auto_after = 0;
	attachment = 0;
	is_exit = 0;
//printf("VirtualNode::VirtualNode 1\n");
}

VirtualNode::~VirtualNode()
{
	subnodes.remove_all_objects();
//printf("VirtualNode::VirtualNode 2\n");
}

#define PRINT_INDENT for(int j = 0; j < indent; j++) printf(" ");

void VirtualNode::dump(int indent)
{
	PRINT_INDENT
	printf("VirtualNode %p title=%s real_module=%p %s\n", 
		this, 
		track->title,
		real_module,
		is_exit ? "*" : " ");
	if(real_module)
	{
		PRINT_INDENT
		printf(" Plugins total=%d\n", subnodes.total);
		for(int i = 0; i < subnodes.total; i++)
		{
			subnodes.values[i]->dump(indent + 2);
		}
	}
	else
	if(real_plugin)
	{
		printf("    Plugin %s\n", real_plugin->title);
	}
}

int VirtualNode::expand(int persistent_plugins, int64_t current_position)
{
// module needs to know where the input data for the next process is
	if(real_module)
	{
		expand_as_module(persistent_plugins, current_position);
	}
	else
	if(real_plugin)
	{
// attach to a real plugin for a plugin
// plugin always takes data from input to output
		expand_as_plugin(persistent_plugins);
	}

	return 0;
}

int VirtualNode::expand_as_module(int duplicate, int64_t current_position)
{
	Transition *transition = 0;

// create the plugins for this module
	for(int i = 0; i < track->plugin_set.total; i++)
	{
		Plugin *plugin = track->get_current_plugin(current_position, 
			i, 
			renderengine->command->get_direction(),
			0,
			1);

// Switch off if circular reference.  This happens if a plugin set or a track is deleted.
		if(plugin == real_plugin) continue;


		if(plugin && plugin->on)
		{
			int plugin_type = plugin->plugin_type;
			if(plugin_type == PLUGIN_SHAREDMODULE)
			{
// plugin is a module
				attach_virtual_module(plugin,
					i, 
					duplicate, 
					current_position);
			}
			else
			if(plugin_type == PLUGIN_SHAREDPLUGIN ||
				plugin_type == PLUGIN_STANDALONE)
			{
// plugin is a plugin
				attach_virtual_plugin(plugin, 
					i, 
					duplicate, 
					current_position);
			}
		}
	}


	if(!parent_node) vconsole->append_exit_node(this);

	return 0;
}

int VirtualNode::expand_as_plugin(int duplicate)
{
	plugin_type = real_plugin->plugin_type;

	if(plugin_type == PLUGIN_SHAREDPLUGIN)
	{
// Attached to a shared plugin.
// Get the real real plugin it's attached to.

// Redirect the real_plugin to the shared plugin.
		int real_module_number = real_plugin->shared_location.module;
		int real_plugin_number = real_plugin->shared_location.plugin;
		Module *real_module = vconsole->module_number(real_module_number);
		real_plugin = 0;
// module references are absolute so may get the wrong data type track.
		if(real_module)
		{
			attachment = real_module->get_attachment(real_plugin_number);
// Attachment is NULL if off
			if(attachment)
			{
				real_plugin = attachment->plugin;

// Real plugin not on then null it.
				if(!real_plugin || !real_plugin->on) real_plugin = 0;
			}
		}
	}






	if(plugin_type == PLUGIN_STANDALONE)
	{
// Get plugin server
		Module *module = vconsole->module_of(track);

		attachment = module->attachment_of(real_plugin);
	}





// Add to real plugin's list of virtual plugins for configuration updates
// and plugin_server initializations.
// Input and output are taken care of when the parent module creates this plugin.
// Get number for passing to render.
// real_plugin may become NULL after shared plugin test.
	if(real_plugin && attachment)
	{
		if(attachment)
			plugin_buffer_number = attachment->attach_virtual_plugin(this);
	}




	return 0;
}

int VirtualNode::attach_virtual_module(Plugin *plugin, 
	int plugin_number, 
	int duplicate, 
	int64_t current_position)
{
	if(plugin->on)
	{
		int real_module_number = plugin->shared_location.module;
		Module *real_module = vconsole->module_number(real_module_number);
// If a track is deleted real_module is not found
		if(!real_module) return 1;

		Track *track = real_module->track;

// Switch off if circular reference.  This happens if a track is deleted.
		if(this->real_module && track == this->real_module->track) return 1;




		VirtualNode *virtual_module = create_module(plugin,
			real_module,
			track);

		subnodes.append(virtual_module);
		virtual_module->expand(duplicate, current_position);
	}

	return 0;
}

int VirtualNode::attach_virtual_plugin(Plugin *plugin, 
	int plugin_number, 
	int duplicate, 
	int64_t current_position)
{
// Get real plugin and test if it is on.
	int is_on = 1;
	if(plugin->plugin_type == PLUGIN_SHAREDPLUGIN)
	{
		int real_module_number = plugin->shared_location.module;
		int real_plugin_number = plugin->shared_location.plugin;
		Module *real_module = vconsole->module_number(real_module_number);
		if(real_module)
		{
			AttachmentPoint *attachment = real_module->get_attachment(real_plugin_number);
			if(attachment)
			{
				Plugin *real_plugin = attachment->plugin;
				if(!real_plugin || !real_plugin->on)
					is_on = 0;
			}
			else
				is_on = 0;
		}
		else
			is_on = 0;
	}

	if(is_on)
	{
		VirtualNode *virtual_plugin = create_plugin(plugin);
		subnodes.append(virtual_plugin);
		virtual_plugin->expand(duplicate, current_position);
	}
	return 0;
}

VirtualNode* VirtualNode::get_previous_plugin(VirtualNode *current_node)
{
	for(int i = 0; i < subnodes.total; i++)
	{
// Assume plugin is on
		if(subnodes.values[i] == current_node)
		{
			if(i > 0) 
				return subnodes.values[i - 1];
			else
				return 0;
		}
	}
	return 0;
}



void VirtualNode::get_mute_fragment(int64_t input_position,
				int &mute_constant, 
				int &fragment_len, 
				Autos *autos,
				int direction,
				int use_nudge)
{
	if(use_nudge) input_position += track->nudge;

	IntAuto *prev_keyframe = 0;
	IntAuto *next_keyframe = 0;
	prev_keyframe = (IntAuto*)autos->get_prev_auto(input_position, 
		direction, 
		(Auto* &)prev_keyframe);
	next_keyframe = (IntAuto*)autos->get_next_auto(input_position, 
		direction, 
		(Auto* &)next_keyframe);

	if(direction == PLAY_FORWARD)
	{
// Two distinct keyframes within range
		if(next_keyframe->position > prev_keyframe->position)
		{
			mute_constant = prev_keyframe->value;

			if(next_keyframe->position < input_position + fragment_len)
				fragment_len = next_keyframe->position - input_position;
		}
		else
// One keyframe within range
		{
			mute_constant = prev_keyframe->value;
		}
	}
	else
	{
// Two distinct keyframes within range
		if(next_keyframe->position < prev_keyframe->position)
		{
			mute_constant = next_keyframe->value;

			if(next_keyframe->position > input_position - fragment_len)
				fragment_len = input_position - next_keyframe->position;
		}
		else
// One keyframe within range
		{
			mute_constant = next_keyframe->value;
		}
	}
}






