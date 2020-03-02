
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

#include "aframe.h"
#include "autoconf.h"
#include "bcsignals.h"
#include "bcresources.h"
#include "cwindow.h"
#include "edl.h"
#include "edlsession.h"
#include "language.h"
#include "localsession.h"
#include "maincursor.h"
#include "mainerror.h"
#include "mainundo.h"
#include "mwindow.h"
#include "playbackengine.h"
#include "plugin.h"
#include "pluginaclient.h"
#include "pluginclient.h"
#include "pluginserver.h"
#include "pluginvclient.h"
#include "preferences.h"
#include "renderengine.inc"
#include "tmpframecache.h"
#include "track.h"
#include "vframe.h"
#include "videodevice.h"

#include <sys/types.h>
#include <sys/wait.h>
#include <dlfcn.h>

PluginServer::PluginServer(const char *path)
{
	reset_parameters();
	this->path = new char[strlen(path) + 1];
	strcpy(this->path, path);
}

PluginServer::PluginServer(PluginServer &that)
{
	reset_parameters();

	if(!that.plugin_fd)
		that.load_plugin();

	if(that.title)
	{
		title = new char[strlen(that.title) + 1];
		strcpy(title, that.title);
	}

	realtime = that.realtime;
	multichannel = that.multichannel;
	synthesis = that.synthesis;
	apiversion = that.apiversion;
	audio = that.audio;
	video = that.video;
	theme = that.theme;
	uses_gui = that.uses_gui;
	keyframe = that.keyframe;
	plugin_fd = that.plugin_fd;
	new_plugin = that.new_plugin;
#ifdef HAVE_LADSPA
	is_lad = that.is_lad;
	ladspa_index = that.ladspa_index;
	lad_descriptor = that.lad_descriptor;
	lad_descriptor_function = that.lad_descriptor_function;
#endif
}

PluginServer::~PluginServer()
{
	close_plugin();
	if(path) delete [] path;
	if(title) delete [] title;
	if(picon) delete picon;
}

// Done only once at creation
int PluginServer::reset_parameters()
{
	keyframe = 0;
	prompt = 0;
	cleanup_plugin();
	plugin_fd = 0;
	plugin = 0;
	title = 0;
	path = 0;
	audio = video = theme = 0;
	uses_gui = 0;
	realtime = multichannel = 0;
	synthesis = 0;
	apiversion = 0;
	picon = 0;
	transition = 0;
	new_plugin = 0;
	client = 0;
	use_opengl = 0;
	vdevice = 0;
#ifdef HAVE_LADSPA
	is_lad = 0;
	ladspa_index = -1;
	lad_descriptor_function = 0;
	lad_descriptor = 0;
#endif
}

// Done every time the plugin is opened or closed
int PluginServer::cleanup_plugin()
{
	total_in_buffers = 0;
	gui_on = 0;
	plugin = 0;
	plugin_open = 0;
}

void PluginServer::set_keyframe(KeyFrame *keyframe)
{
	this->keyframe = keyframe;
}

void PluginServer::set_prompt(MenuEffectPrompt *prompt)
{
	this->prompt = prompt;
}

void PluginServer::set_title(const char *string)
{
	if(title) delete [] title;
	title = new char[strlen(string) + 1];
	strcpy(title, string);
}

void PluginServer::generate_display_title(char *string)
{
	char lbuf[BCTEXTLEN];

	if(BC_Resources::locale_utf8)
		strcpy(lbuf, _(title));
	else
		BC_Resources::encode(BC_Resources::encoding, 0, _(title), lbuf, BCTEXTLEN);

	if(plugin && plugin->track) 
		sprintf(string, "%s - %s", lbuf, plugin->track->title);
	else
		sprintf(string, "%s - %s", lbuf, PROGRAM_NAME);
}

// Open plugin for signal processing
PluginClient *PluginServer::open_plugin(Plugin *plugin,
	TrackRender *renderer, int master, int lad_index)
{
	if(plugin_open)
		return client;

	this->plugin = plugin;

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
	multichannel = client->is_multichannel();
	synthesis = client->is_synthesis();
	apiversion = client->has_pts_api();
	transition = client->is_transition();
	set_title(client->plugin_title());
	client->set_renderer(renderer);
	client->plugin = plugin;

// Check API version
	if((audio || video) && !apiversion)
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

	plugin_open = 1;
	return client;
}

void PluginServer::close_plugin()
{
	if(!plugin_open) return;

	delete client;
	if(plugin)
		plugin->client = 0;
	client = 0;
	plugin_open = 0;
	cleanup_plugin();
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

void PluginServer::update_title()
{
	if(!plugin_open) return;
	client->update_display_title();
}

void PluginServer::set_use_opengl(int value, VideoDevice *vdevice)
{
// Disable until opengl rewrite is ready
//	this->use_opengl = value;
	this->use_opengl = 0;
	this->vdevice = vdevice;
}

int PluginServer::get_use_opengl()
{
	return use_opengl;
}

void PluginServer::run_opengl(PluginClient *plugin_client)
{
	// Should not happen
	printf("PluginServer::run_opengl called\n");
}

// ============================= queries

void PluginServer::save_data(KeyFrame *keyframe)
{
	if(!plugin_open) return;
	client->save_data(keyframe);
}

KeyFrame* PluginServer::get_keyframe()
{
	if(plugin)
		return plugin->get_keyframe(master_edl->local_session->get_selectionstart(1));
	else
		return keyframe;
}

Theme* PluginServer::new_theme()
{
	if(theme)
	{
		return client->new_theme();
	}
	else
		return 0;
}

void PluginServer::dump(int indent)
{
	printf("%*sPluginServer '%s' %s\n", indent, "", title, path);
}
