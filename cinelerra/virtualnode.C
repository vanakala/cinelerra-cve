
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
#include "bcsignals.h"
#include "intauto.h"
#include "intautos.h"
#include "module.h"
#include "plugin.h"
#include "renderengine.h"
#include "track.h"
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
	plugin_type = 0;
	plugin_buffer_number = 0;
	attachment = 0;
}

VirtualNode::~VirtualNode()
{
	subnodes.remove_all_objects();
}

void VirtualNode::dump(int indent)
{
	printf("%*sVirtualNode %p track: '%s' real_module: %p\n", indent, "",
		this, 
		track->title,
		real_module);
	indent += 2;

	if(real_module)
	{
		real_module->dump(indent + 2);
		printf("%*sTotal subnodes: %d\n", indent, "", subnodes.total);
		for(int i = 0; i < subnodes.total; i++)
		{
			subnodes.values[i]->dump(indent + 2);
		}
	}
	else
	if(real_plugin)
	{
		printf("%*sPlugin: '%s'\n", indent, "", real_plugin->title);
	}
}

void VirtualNode::expand(int persistent_plugins, ptstime current_position)
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
}

void VirtualNode::expand_as_module(int duplicate, ptstime current_postime)
{
// create the plugins for this module
	for(int i = 0; i < track->plugin_set.total; i++)
	{
		Plugin *plugin = track->get_current_plugin(current_postime,
			i, 
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
					current_postime);
			}
			else
			if(plugin_type == PLUGIN_SHAREDPLUGIN ||
				plugin_type == PLUGIN_STANDALONE)
			{
// plugin is a plugin
				attach_virtual_plugin(plugin, 
					i, 
					duplicate, 
					current_postime);
			}
		}
	}

	if(!parent_node) vconsole->append_exit_node(this);
}

void VirtualNode::expand_as_plugin(int duplicate)
{
	plugin_type = real_plugin->plugin_type;

	if(plugin_type == PLUGIN_SHAREDPLUGIN)
	{
// Attached to a shared plugin.
// Get the real real plugin it's attached to.
		real_plugin = real_plugin->shared_plugin;
// Real plugin not on then null it.
		if(real_plugin && !real_plugin->on)
			real_plugin = 0;
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
}

void VirtualNode::attach_virtual_module(Plugin *plugin,
	int plugin_number, 
	int duplicate, 
	ptstime current_postime)
{
	if(plugin->on)
	{
		int real_module_number = plugin->shared_track->number_of();
		Module *real_module = vconsole->module_number(real_module_number);
// If a track is deleted real_module is not found
		if(!real_module) return;

		Track *track = real_module->track;

// Switch off if circular reference.  This happens if a track is deleted.
		if(this->real_module && track == this->real_module->track)
			return;

		VirtualNode *virtual_module = create_module(plugin,
			real_module,
			track);

		subnodes.append(virtual_module);
		virtual_module->expand(duplicate, current_postime);
	}
}

void VirtualNode::attach_virtual_plugin(Plugin *plugin,
	int plugin_number, 
	int duplicate, 
	ptstime current_postime)
{
// Get real plugin and test if it is on.
	int is_on = 1;

	if(plugin->plugin_type == PLUGIN_SHAREDPLUGIN)
	{
		if(!plugin->shared_plugin || !plugin->shared_plugin->on)
			is_on = 0;
	}

	if(is_on)
	{
		VirtualNode *virtual_plugin = create_plugin(plugin);
		subnodes.append(virtual_plugin);
		virtual_plugin->expand(duplicate, current_postime);
	}
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

void VirtualNode::get_mute_fragment(ptstime input_position,
				int &mute_constant, 
				ptstime &fragment_len, 
				Autos *autos)
{
	IntAuto *prev_keyframe = 0;
	IntAuto *next_keyframe = 0;

	prev_keyframe = (IntAuto*)autos->get_prev_auto(input_position, 
		(Auto* &)prev_keyframe);
	next_keyframe = (IntAuto*)autos->get_next_auto(input_position, 
		(Auto* &)next_keyframe);

	if(!prev_keyframe && !next_keyframe)
	{
		mute_constant = ((IntAutos*)autos)->get_value(input_position);
		return;
	}

// Two distinct keyframes within range
	if(next_keyframe->pos_time > prev_keyframe->pos_time)
	{
		mute_constant = prev_keyframe->value;

		if(next_keyframe->pos_time < input_position + fragment_len)
			fragment_len = next_keyframe->pos_time - input_position;
	}
	else
// One keyframe within range
	{
		mute_constant = prev_keyframe->value;
	}
}
