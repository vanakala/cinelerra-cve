#include "attachmentpoint.h"
#include "filexml.h"
#include "edl.h"
#include "edlsession.h"
#include "mwindow.h"
#include "plugin.h"
#include "pluginserver.h"
#include "renderengine.h"
#include "transportque.h"
#include "virtualnode.h"



AttachmentPoint::AttachmentPoint(RenderEngine *renderengine, Plugin *plugin)
{
	reset_parameters();
	this->plugin = plugin;
	this->renderengine = renderengine;
	plugin_server = renderengine->scan_plugindb(plugin->title);
//printf("AttachmentPoint::AttachmentPoint %s %p\n", plugin->title, plugin_server);
}

AttachmentPoint::~AttachmentPoint()
{
	delete_buffer_vectors();
	plugin_servers.remove_all_objects();
}


int AttachmentPoint::reset_parameters()
{
	current_buffer = 0;
//	plugin_type = 0;
	plugin_server = 0;
//	plugin_type = 0;
	new_total_input_buffers = 0;
	total_input_buffers = 0;
	return 0;
}

int AttachmentPoint::render_init()
{
//printf("AttachmentPoint::render_init 1\n");
	if(plugin_server && plugin->on)
	{
// Start new plugins or continue existing plugins
// Create new plugin servers
//printf("AttachmentPoint::render_init 2 %d %d\n", virtual_plugins.total, new_virtual_plugins.total);
		if(virtual_plugins.total != new_virtual_plugins.total)
		{
//printf("AttachmentPoint::render_init 3\n");
			plugin_servers.remove_all_objects();
//printf("AttachmentPoint::render_init 4\n");
			for(int i = 0; i < new_virtual_plugins.total; i++)
			{
//printf("AttachmentPoint::render_init 5\n");
				if(i == 0 || !plugin_server->multichannel)
				{
//printf("AttachmentPoint::render_init 5.1\n");
					PluginServer *new_server;
					plugin_servers.append(new_server = new PluginServer(*plugin_server));
					new_server->set_attachmentpoint(this);
//printf("AttachmentPoint::render_init 5.2 %p %p\n", plugin_servers.values[i], plugin);
					plugin_servers.values[i]->open_plugin(0, renderengine->edl, plugin);
//printf("AttachmentPoint::render_init 5.3\n");
					plugin_servers.values[i]->init_realtime(
						renderengine->edl->session->real_time_playback &&
							renderengine->command->realtime,
						new_total_input_buffers,
						get_buffer_size());
//printf("AttachmentPoint::render_init 5.4\n");
				}
//printf("AttachmentPoint::render_init 6\n");
			}
		}
	}

//printf("AttachmentPoint::render_init 7\n");
	virtual_plugins.remove_all();
	for(int i = 0; i < new_virtual_plugins.total; i++)
		virtual_plugins.append(new_virtual_plugins.values[i]);
	new_virtual_plugins.remove_all();
//printf("AttachmentPoint::render_init 8\n");

	total_input_buffers = new_total_input_buffers;
	new_total_input_buffers = 0;
//printf("AttachmentPoint::render_init 9\n");

	delete_buffer_vectors();
	new_buffer_vectors();
//printf("AttachmentPoint::render_init 10\n");
	return 0;
}

int AttachmentPoint::render_stop(int duplicate)
{
// stop plugins
// Can't use the on value here because it may have changed.
//printf("AttachmentPoint::render_stop 1\n");
	if(plugin_server && plugin->on && virtual_plugins.total && !duplicate)
	{
// close the plugins if not shared
		for(int i = 0; i < virtual_plugins.total; i++)
		{
			if(i == 0 || !plugin_server->multichannel)
			{
//printf("AttachmentPoint::render_stop 2\n");
				plugin_servers.values[i]->close_plugin();
			}
		}
//printf("AttachmentPoint::render_stop 3\n");
		plugin_servers.remove_all_objects();
	}
	return 0;
}

int AttachmentPoint::attach_virtual_plugin(VirtualNode *virtual_plugin)
{
	int in_buffer_number = 0;

// add virtual plugin to list of new virtual plugins
	new_virtual_plugins.append(virtual_plugin);

// Increment input buffer counter
	if(plugin_server && plugin->on)
	{
		in_buffer_number = new_total_input_buffers++;
	}
//printf("AttachmentPoint::attach_virtual_plugin %p %d %d %d\n", plugin_server, plugin->on, in_buffer_number, new_total_input_buffers);

	return in_buffer_number;
}

int AttachmentPoint::sort(VirtualNode *virtual_plugin)
{
	int result = 0;

	for(int i = 0; i < new_virtual_plugins.total && !result; i++)
	{
// if no virtual plugins attached to this are waiting return 1
		if(!new_virtual_plugins.values[i]->waiting_real_plugin) result = 1;
	}

	return result;
}

int AttachmentPoint::multichannel_shared(int search_new)
{
	if(search_new)
	{
		if(new_virtual_plugins.total && 
			plugin_server && 
			plugin_server->multichannel) return 1;
	}
	else
	{
		if(virtual_plugins.total && 
			plugin_server && 
			plugin_server->multichannel) return 1;
	}
	return 0;
}

int AttachmentPoint::singlechannel()
{
	if(plugin_server && !plugin_server->multichannel) return 1;
	return 0;
}

void AttachmentPoint::render(long current_position, long fragment_size)
{
// All buffers must be armed before proceeding
//printf("AttachmentPoint::render %p %d %d %d\n", plugin_server, plugin_server->multichannel, current_buffer, total_input_buffers - 1);
	if(plugin_server)
	{
		if(current_buffer == total_input_buffers - 1 ||
			!plugin_server->multichannel)
		{
				dispatch_plugin_server(current_buffer, 
					current_position, 
					fragment_size);
		}
	}

	current_buffer++;
	if(current_buffer >= total_input_buffers) current_buffer = 0;
}


void AttachmentPoint::render_gui(void *data)
{
//printf("AttachmentPoint::render_gui 1 %p %p\n", renderengine, renderengine->mwindow);
	if(renderengine && renderengine->mwindow)
		renderengine->mwindow->render_plugin_gui(data, plugin);
}

void AttachmentPoint::render_gui(void *data, int size)
{
	if(renderengine && renderengine->mwindow)
		renderengine->mwindow->render_plugin_gui(data, size, plugin);
}













int AttachmentPoint::dump()
{
	printf("    Attachmentpoint %x\n", this);
	if(plugin_server) plugin_server->dump();
}




