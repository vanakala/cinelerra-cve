#include "attachmentpoint.h"
#include "commonrender.h"
#include "edl.h"
#include "edlsession.h"
#include "filexml.h"
#include "module.h"
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
//printf("Module::Module 1\n");
}

Module::~Module()
{
//printf("Module::~Module 1\n");
	if(attachments)
	{
		for(int i = 0; i < track->plugin_set.total; i++)
		{
//printf("Module::~Module 2 %p\n", attachments[i]);
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
// Not needed in pluginarray
	if(commonrender)
	{
//printf("Module::create_new_attachments 1\n");
		new_total_attachments = track->plugin_set.total;
		new_attachments = new AttachmentPoint*[new_total_attachments];
//printf("Module::create_new_attachments 2\n");

// Create plugin servers later when nodes attach
		for(int i = 0; i < new_total_attachments; i++)
		{
//printf("Module::create_new_attachments 3\n");
			Plugin *plugin = 
				track->get_current_plugin(commonrender->current_position, 
					i, 
					renderengine->command->get_direction(),
					0);
//printf("Module::create_new_attachments 4\n");

			if(plugin && plugin->plugin_type != PLUGIN_NONE && plugin->on)
				new_attachments[i] = new_attachment(plugin);
			else
				new_attachments[i] = 0;
//printf("Module::create_new_attachments 5 %p %p\n", plugin, new_attachments[i]);
		}
//printf("Module::create_new_attachments 6\n");
	}
}

void Module::swap_attachments()
{
//printf("Module::swap_attachments 1\n");
// None of this is used in a pluginarray
	for(int new_attachment = 0, old_attachment = 0; 
		new_attachment < new_total_attachments &&
		old_attachment < total_attachments; 
		new_attachment++, old_attachment++)
	{
// Copy any old attachment which is identical to a new one
		if(new_attachments[new_attachment] &&
			attachments[old_attachment] &&
			new_attachments[new_attachment]->plugin ==
			attachments[old_attachment]->plugin)
		{
			delete new_attachments[new_attachment];
			new_attachments[new_attachment] = attachments[old_attachment];
			attachments[old_attachment] = 0;
		}
	}
//printf("Module::swap_attachments 1 %d\n", total_attachments);

// Delete old attachments which weren't identical to new ones
	for(int i = 0; i < total_attachments; i++)
	{
		if(attachments[i]) delete attachments[i];
	}
//printf("Module::swap_attachments 1\n");
	if(attachments) delete attachments;
//printf("Module::swap_attachments 1\n");
	
	if(new_total_attachments)
	{
		attachments = new_attachments;
		total_attachments = new_total_attachments;
	}
//printf("Module::swap_attachments 2\n");
}

int Module::render_init()
{
//printf("Module::render_init 1\n");
	for(int i = 0; i < total_attachments; i++)
	{
		if(attachments[i])
			attachments[i]->render_init();
	}
//printf("Module::render_init 2\n");
	return 0;
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


// Test plugins for reconfiguration
int Module::test_plugins()
{
//printf("Module::test_plugins 1\n");
	if(total_attachments != track->plugin_set.total) return 1;
//printf("Module::test_plugins 2\n");

	for(int i = 0; i < total_attachments; i++)
	{
		AttachmentPoint *attachment = attachments[i];
		Plugin *plugin = track->get_current_plugin(commonrender->current_position, 
			i, 
			renderengine->command->get_direction(),
			0);
//printf("Module::test_plugins 3 %p %p\n", attachment, plugin);
// One exists and one doesn't
		int use_plugin = plugin &&
			plugin->plugin_type != PLUGIN_NONE &&
			plugin->on;

		if((attachment && !use_plugin) || 
			(!attachment && use_plugin)) return 1;
//printf("Module::test_plugins 4 %p %p\n", attachment, plugin);

// Plugin not the same
		if(plugin && 
			attachment &&
			attachment->plugin && 
			!plugin->identical(attachment->plugin)) return 1;
	}
//printf("Module::test_plugins 5\n");

	return 0;
}

void Module::update_transition(long current_position, int direction)
{
//printf("Module::update_transition 1\n");
	Plugin *transition = track->get_current_transition(current_position, 
		direction,
		0);
//printf("Module::update_transition 2 %p\n", transition);

	if((!transition && this->transition) || 
		(transition && this->transition && strcmp(transition->title, this->transition->title)))
	{
		this->transition = 0;

		transition_server->close_plugin();
		delete transition_server;
		transition_server = 0;
	}
//printf("Module::update_transition 7\n");

	if(transition && !this->transition)
	{
		this->transition = transition;
//printf("Module::update_transition 7\n");
		if(renderengine)
		{
			PluginServer *plugin_server = renderengine->scan_plugindb(transition->title);
			transition_server = new PluginServer(*plugin_server);
			transition_server->open_plugin(0, get_edl(), transition);
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
			transition_server->open_plugin(0, get_edl(), transition);
			transition_server->init_realtime(
				0,
				1,
				get_buffer_size());
		}
//printf("Module::update_transition 7\n");
	}

//printf("Module::update_transition 8\n");
// Test transition
//	
//	if((transition && !this->transition) || (!transition && this->transition)) return 1;
//	if(transition && transition != this->transition->plugin) return 1;
}


void Module::dump()
{
	printf("  Module\n");
	printf("   Plugins\n");
	for(int i = 0; i < total_attachments; i++)
	{
		if(attachments[i])
			attachments[i]->dump();
		else
			printf("    No Plugin\n");
	}
}






