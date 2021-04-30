
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

#include "pluginaclientlad.h"

#include "bcsignals.h"
#include "plugin.h"
#include "pluginclient.h"
#include "pluginserver.h"
#include "vframe.h"

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dlfcn.h>

PluginServer::PluginServer(const char *path)
{
	reset_parameters();
	this->path = new char[strlen(path) + 1];
	strcpy(this->path, path);
}

PluginServer::~PluginServer()
{
	delete [] path;
	delete [] title;
	delete picon;
}

// Done only once at creation
void PluginServer::reset_parameters()
{
	plugin_fd = 0;
	title = 0;
	path = 0;
	audio = video = theme = 0;
	uses_gui = 0;
	status_gui = 0;
	realtime = multichannel = 0;
	synthesis = 0;
	apiversion = 0;
	picon = 0;
	transition = 0;
	new_plugin = 0;
#ifdef HAVE_LADSPA
	is_lad = 0;
	ladspa_index = -1;
	lad_descriptor_function = 0;
	lad_descriptor = 0;
#endif
}

void PluginServer::set_title(const char *string)
{
	if(title) delete [] title;
	title = new char[strlen(string) + 1];
	strcpy(title, string);
}

// Open plugin for signal processing
PluginClient *PluginServer::open_plugin(Plugin *plugin,
	TrackRender *renderer, int master, int lad_index)
{
	PluginClient *client;

	if(!plugin_fd)
	{
#ifdef HAVE_LADSPA
		if(lad_index >= 0)
			ladspa_index = lad_index;
#endif
		if(load_plugin())
			return 0;
	}

#ifdef HAVE_LADSPA
	if(is_lad)
	{
		client = new PluginAClientLAD(this);
	}
	else
#endif
	{
		client = new_plugin(this);
	}
	realtime = client->is_realtime();
	audio = client->is_audio();
	video = client->is_video();
	theme = client->is_theme();
	uses_gui = client->uses_gui();
	status_gui = client->has_status_gui();
	multichannel = client->is_multichannel();
	synthesis = client->is_synthesis();
	no_keyframes = client->not_keyframeable();
	apiversion = client->api_version();
	transition = client->is_transition();
	opengl_plugin = client->has_opengl_support();
	multichannel_max = client->multi_max_channels();
	set_title(client->plugin_title());
	client->set_renderer(renderer);
	client->plugin = plugin;

// Check API version
	if((audio || video) && apiversion < 3)
	{
		delete client;
		dlclose(plugin_fd);
		plugin_fd = 0;
		fprintf(stderr, "Old version plugin: %s\n", path);
		return 0;
	}
	if(plugin)
		plugin->client = client;

	if(master)
	{
		picon = client->new_picon();
	}

	return client;
}

void PluginServer::close_plugin(PluginClient *client)
{
	if(!client)
		return;

	if(client->plugin)
	{
		if(client->plugin->client == client)
		{
			client->plugin->client = 0;
			client->plugin->show = 0;
		}
	}
	delete client;
}

int PluginServer::load_plugin()
{
	if(plugin_fd)
		return 0;

	if(!(plugin_fd = dlopen(path, RTLD_NOW)))
	{
		fprintf(stderr, "open_plugin: %s\n", dlerror());
		return 1;
	}
#ifdef HAVE_LADSPA
	if(!is_lad)
#endif
		new_plugin = (PluginClient* (*)(PluginServer*))dlsym(plugin_fd, "new_plugin");
	if(!new_plugin)
	{
#ifdef HAVE_LADSPA
		lad_descriptor_function = (LADSPA_Descriptor_Function)dlsym(
			plugin_fd, "ladspa_descriptor");
		if(!lad_descriptor_function)
		{
			fprintf(stderr, "Unrecognized plugin: %s\n", path);
			dlclose(plugin_fd);
			plugin_fd = 0;
			return 1;
		}
		is_lad = 1;

		if(ladspa_index >= 0)
			lad_descriptor = lad_descriptor_function(ladspa_index);
// Make plugin initializer handle the subplugins in the LAD plugin or stop
//  trying subplugins.
		if(!lad_descriptor)
		{
			dlclose(plugin_fd);
			plugin_fd = 0;
			return 1;
		}
#else
// Not a recognized plugin
		fprintf(stderr, "Unrecognized plugin %s\n", path);
		dlclose(plugin_fd);
		plugin_fd = 0;
		return 1;
#endif
	}
	return 0;
}

void PluginServer::release_plugin()
{
	if(!plugin_fd)
		return;

	dlclose(plugin_fd);
	plugin_fd = 0;
}

void PluginServer::dump(int indent)
{
	printf("%*sPluginServer '%s' %s\n", indent, "", title, path);
}
