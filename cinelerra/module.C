
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
#include "bcsignals.h"
#include "commonrender.h"
#include "edl.h"
#include "edlsession.h"
#include "filexml.h"
#include "module.h"
#include "mwindow.h"
#include "patchbay.h"
#include "plugin.h"
#include "pluginarray.h"
#include "pluginserver.h"
#include "renderengine.h"
#include "track.h"
#include "tracks.h"

Module::Module(RenderEngine *renderengine, 
	CommonRender *commonrender, 
	PluginArray *plugin_array,
	Track *track)
{
	this->renderengine = renderengine;
	this->commonrender = commonrender;
	this->plugin_array = plugin_array;
	this->track = track;
	transition = 0;
	transition_server = 0;
	attachments = 0;
	total_attachments = 0;
	new_total_attachments = 0;
	new_attachments = 0;
	reset();
}

Module::~Module()
{
	if(attachments)
	{
		for(int i = 0; i < total_attachments; i++)
		{
			if(attachments[i])
			{
// For some reason it isn't used here.
//				attachments[i]->render_stop(0);
				delete attachments[i];
			}
		}
		delete [] attachments;
	}
	if(transition_server)
	{
		transition_server->close_plugin();
		delete transition_server;
	}
}

void Module::reset()
{
	create_new_attachments();
	swap_attachments();
}

EDL* Module::get_edl()
{
	if(renderengine) 
		return renderengine->edl;
	else
		return edl;
}

void Module::create_new_attachments()
{
// Not used in pluginarray
	if(commonrender)
	{
		new_total_attachments = track->plugins.total;
		if(new_total_attachments)
		{
			new_attachments = new AttachmentPoint*[new_total_attachments];
			for(int i = 0; i < new_total_attachments; i++)
			{
				Plugin *plugin = 
					track->get_current_plugin(commonrender->current_postime,
						i, 
						1);

				if(plugin && plugin->plugin_type != PLUGIN_NONE && plugin->on)
					new_attachments[i] = new_attachment(plugin);
				else
					new_attachments[i] = 0;
			}
		}
		else
			new_attachments = 0;

// Create plugin servers in virtual console expansion
	}
}

void Module::swap_attachments()
{
// None of this is used in a pluginarray
	for(int i = 0; 
		i < new_total_attachments &&
		i < total_attachments; 
		i++)
	{
// Delete new attachment which is identical to the old one and copy
// old attachment.
		if(new_attachments[i] &&
			attachments[i] &&
			new_attachments[i]->identical(attachments[i]))
		{
			delete new_attachments[i];
			new_attachments[i] = attachments[i];
			attachments[i] = 0;
		}
	}

// Delete old attachments which weren't identical to new ones
	for(int i = 0; i < total_attachments; i++)
	{
		if(attachments[i]) delete attachments[i];
	}

	if(attachments)
	{
		delete [] attachments;
	}

	attachments = new_attachments;
	total_attachments = new_total_attachments;
}

void Module::render_init()
{
	for(int i = 0; i < total_attachments; i++)
	{
		if(attachments[i])
			attachments[i]->render_init();
	}
}

void Module::render_stop()
{
	for(int i = 0; i < total_attachments; i++)
	{
		if(attachments[i])
			attachments[i]->render_stop();
	}
}

AttachmentPoint* Module::attachment_of(Plugin *plugin)
{
	for(int i = 0; i < total_attachments; i++)
	{
		if(attachments[i] && 
			attachments[i]->plugin == plugin) return attachments[i];
	}
	return 0;
}

AttachmentPoint* Module::get_attachment(int number)
{
	if(number < total_attachments)
		return attachments[number];
	else
		return 0;
}

void Module::reset_attachments()
{
	for(int i = 0; i < total_attachments; i++)
	{
		AttachmentPoint *attachment = attachments[i];
		if(attachment) attachment->reset_status();
	}
}

// Test plugins for reconfiguration.
// Used in playback
int Module::test_plugins(void)
{
	if(total_attachments != track->plugins.total)
		return 1;

	for(int i = 0; i < total_attachments; i++)
	{
		AttachmentPoint *attachment = attachments[i];
		Plugin *plugin = track->get_current_plugin(
			commonrender->current_postime,
			i, 
			1);
// One exists and one doesn't
		int use_plugin = plugin &&
			plugin->plugin_type != PLUGIN_NONE &&
			plugin->on;

		if((attachment && !use_plugin) || 
			(!attachment && use_plugin)) return 1;

// Plugin not the same
		if(plugin && 
			attachment &&
			attachment->plugin && 
			!plugin->identical(attachment->plugin)) return 1;
	}

	return 0;
}

void Module::update_transition(ptstime current_position)
{
	transition = track->get_current_transition(current_position);

	if(transition && !transition->plugin_server)
		return;

// For situations where we had a transition but not anymore, 
// keep the server open.
// Maybe the same transition will follow and we won't need to reinit.
// (happens a lot while scrubbing over transitions left and right)


// If the current transition differs from the previous transition, delete the
// server.
	if (transition && 
		transition_server)
	{
		if(transition->plugin_server !=  transition_server->plugin->plugin_server)
		{
			transition_server->close_plugin();
			delete transition_server;
			transition_server = 0;
		} else
		{
			transition_server->plugin = transition;
		}
	}

	if(transition && !transition_server)
	{
		if(renderengine)
		{
			PluginServer *plugin_server = transition->plugin_server;
			transition_server = new PluginServer(*plugin_server);
			transition_server->open_plugin(0, transition, 0);
			transition_server->init_realtime(1);
		}
		else
		if(plugin_array)
		{
			PluginServer *plugin_server = transition->plugin_server;
			transition_server = new PluginServer(*plugin_server);
			transition_server->open_plugin(0, transition, 0);
			transition_server->init_realtime(1);
		}
	}
}

void Module::dump(int indent)
{
	printf("%*sModule %p track: '%s' attachments: %d\n", indent, "", this,
		track->title, total_attachments);
	for(int i = 0; i < total_attachments; i++)
	{
		if(attachments[i])
			attachments[i]->dump(indent + 2);
	}
}
