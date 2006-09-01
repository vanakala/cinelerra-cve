#include "attachmentpoint.h"
#include "bcsignals.h"
#include "commonrender.h"
#include "edl.h"
#include "edlsession.h"
#include "filexml.h"
#include "module.h"
#include "mwindow.h"
#include "patch.h"
#include "patchbay.h"
#include "plugin.h"
#include "pluginarray.h"
#include "pluginserver.h"
#include "renderengine.h"
#include "sharedlocation.h"
#include "track.h"
#include "tracks.h"
#include "transportque.h"


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
}

Module::~Module()
{
	if(attachments)
	{
		for(int i = 0; i < track->plugin_set.total; i++)
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

void Module::create_objects()
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
		new_total_attachments = track->plugin_set.total;
		if(new_total_attachments)
		{
			new_attachments = new AttachmentPoint*[new_total_attachments];
			for(int i = 0; i < new_total_attachments; i++)
			{
				Plugin *plugin = 
					track->get_current_plugin(commonrender->current_position, 
						i, 
						renderengine->command->get_direction(),
						0,
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

int Module::render_init()
{
	for(int i = 0; i < total_attachments; i++)
	{
		if(attachments[i])
			attachments[i]->render_init();
	}

	return 0;
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
//printf("Module::attachment_of 1 %d\n", total_attachments);
	for(int i = 0; i < total_attachments; i++)
	{
//printf("Module::attachment_of 2 %p\n", attachments[i]);
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
//printf("Module::reset_attachments 1 %d\n", total_attachments);
	for(int i = 0; i < total_attachments; i++)
	{
//printf("Module::reset_attachments 2 %p\n", attachments[i]);
		AttachmentPoint *attachment = attachments[i];
		if(attachment) attachment->reset_status();
	}
}

// Test plugins for reconfiguration.
// Used in playback
int Module::test_plugins()
{
	if(total_attachments != track->plugin_set.total) return 1;

	for(int i = 0; i < total_attachments; i++)
	{
		AttachmentPoint *attachment = attachments[i];
		Plugin *plugin = track->get_current_plugin(
			commonrender->current_position, 
			i, 
			renderengine->command->get_direction(),
			0,
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

void Module::update_transition(int64_t current_position, 
	int direction)
{
	transition = track->get_current_transition(current_position, 
		direction,
		0,
		0); // position is already nudged in amodule.C and vmodule.C before calling update_transition!

// for situations where we had transition and have no more, we keep the server open:
// maybe the same transition will follow and we won't need to reinit... (happens a lot while scrubbing over transitions left and right)
//	if((prev_transition && !transition) ||
	if ((transition && transition_server && strcmp(transition->title, transition_server->plugin->title)))
	{
		transition_server->close_plugin();
		delete transition_server;
		transition_server = 0;
	}

	if(transition && !transition_server)
	{
		if(renderengine)
		{
			PluginServer *plugin_server = renderengine->scan_plugindb(transition->title,
				track->data_type);
			transition_server = new PluginServer(*plugin_server);
			transition_server->open_plugin(0, 
				renderengine->preferences, 
				get_edl(), 
				transition,
				-1);
			transition_server->init_realtime(
				get_edl()->session->real_time_playback &&
				renderengine->command->realtime,
				1,
				get_buffer_size());
		}
		else
		if(plugin_array)
		{
			PluginServer *plugin_server = plugin_array->scan_plugindb(transition->title);
			transition_server = new PluginServer(*plugin_server);
			transition_server->open_plugin(0, 
				plugin_array->mwindow->preferences,
				get_edl(), 
				transition,
				-1);
			transition_server->init_realtime(
				0,
				1,
				get_buffer_size());
		}
	}
}


void Module::dump()
{
	printf("  Module title=%s\n", track->title);
	printf("   Plugins total_attachments=%d\n", total_attachments);
	for(int i = 0; i < total_attachments; i++)
	{
		attachments[i]->dump();
	}
}






