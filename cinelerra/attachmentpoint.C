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



AttachmentPoint::AttachmentPoint(RenderEngine *renderengine, 
	Plugin *plugin,
	int data_type)
{
	reset_parameters();
	this->plugin = plugin;
	this->plugin_id = plugin->id;
	this->renderengine = renderengine;
	this->data_type = data_type;
	plugin_server = renderengine->scan_plugindb(plugin->title,
		data_type);
}

AttachmentPoint::~AttachmentPoint()
{
	delete_buffer_vector();
	plugin_servers.remove_all_objects();
}


int AttachmentPoint::reset_parameters()
{
	plugin_server = 0;
	reset_status();
	return 0;
}


void AttachmentPoint::reset_status()
{
	start_position = 0;
	len = 0;
	sample_rate = 0;
	frame_rate = 0;
	is_processed = 0;
}


int AttachmentPoint::identical(AttachmentPoint *old)
{
	return plugin_id == old->plugin_id;
}


int AttachmentPoint::render_init()
{
	if(plugin_server && plugin->on)
	{
// Start new plugin servers if the number of nodes changed.
// The number of nodes can change independantly of the module's 
// attachment count.
		if(virtual_plugins.total != new_virtual_plugins.total)
		{
			plugin_servers.remove_all_objects();
			for(int i = 0; i < new_virtual_plugins.total; i++)
			{
				if(i == 0 || !plugin_server->multichannel)
				{
					PluginServer *new_server;
					plugin_servers.append(new_server = new PluginServer(*plugin_server));
					new_server->set_attachmentpoint(this);
					plugin_servers.values[i]->open_plugin(0, 
						renderengine->preferences,
						renderengine->edl, 
						plugin,
						-1);
					plugin_servers.values[i]->init_realtime(
						renderengine->edl->session->real_time_playback &&
							renderengine->command->realtime,
						plugin_server->multichannel ? new_virtual_plugins.total : 1,
						get_buffer_size());
				}
			}
		}

// Set new back pointers in the plugin servers
		if(plugin_server->multichannel && plugin_servers.total)
		{
			PluginServer *new_server = plugin_servers.values[0];
			new_server->reset_nodes();
			for(int i = 0; i < new_virtual_plugins.total; i++)
			{
				new_server->append_node(new_virtual_plugins.values[i]);
			}
		}
		else
		{
			for(int i = 0; i < new_virtual_plugins.total; i++)
			{
				PluginServer *new_server = plugin_servers.values[i];
				new_server->reset_nodes();
				new_server->append_node(new_virtual_plugins.values[i]);
			}
		}
	}

// Delete old plugin servers
	delete_buffer_vector();
	virtual_plugins.remove_all();

// Set new plugin servers
	for(int i = 0; i < new_virtual_plugins.total; i++)
		virtual_plugins.append(new_virtual_plugins.values[i]);
	new_virtual_plugins.remove_all();


	return 0;
}

int AttachmentPoint::render_stop(int duplicate)
{
// stop plugins
// Can't use the on value here because it may have changed.
	if(plugin_server && plugin->on && virtual_plugins.total && !duplicate)
	{
// close the plugins if not shared
		for(int i = 0; i < virtual_plugins.total; i++)
		{
			if(i == 0 || !plugin_server->multichannel)
			{
				plugin_servers.values[i]->close_plugin();
			}
		}
		plugin_servers.remove_all_objects();
	}
	return 0;
}

int AttachmentPoint::attach_virtual_plugin(VirtualNode *virtual_plugin)
{
	int buffer_number = 0;

	if(plugin_server && plugin->on)
	{
// add virtual plugin to list of new virtual plugins
		new_virtual_plugins.append(virtual_plugin);
//printf("AttachmentPoint::attach_virtual_plugin 1 %d\n", new_virtual_plugins.total);
// Always increment buffer number since this also corresponds to what 
// plugin server to access if single channel.
		buffer_number = new_virtual_plugins.total - 1;
	}
	else
	if(!plugin->on)
	{
		printf("AttachmentPoint::attach_virtual_plugin attempt to attach plugin when off.\n");
	}
	else
	if(!plugin_server)
	{
		printf("AttachmentPoint::attach_virtual_plugin attempt to attach when no plugin_server.\n");
	}

	return buffer_number;
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


void AttachmentPoint::render_gui(void *data)
{
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
	printf("    Attachmentpoint %x virtual_plugins=%d\n", this, new_virtual_plugins.total);
	if(plugin_server) plugin_server->dump();
}




