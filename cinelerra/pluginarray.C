
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

#include "atrack.h"
#include "cache.h"
#include "edl.h"
#include "edlsession.h"
#include "file.h"
#include "keyframe.h"
#include "language.h"
#include "mainprogress.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "pluginarray.h"
#include "pluginserver.h"
#include "preferences.h"
#include "bcprogressbox.h"




PluginArray::PluginArray(int data_type)
 : ArrayList<PluginServer*>()
{
	this->data_type = data_type;
}

PluginArray::~PluginArray()
{
	remove_all_objects();
	delete [] modules;
}


PluginServer* PluginArray::scan_plugindb(char *title)
{
	return mwindow->scan_plugindb(title, data_type);
}

int PluginArray::start_plugins(MWindow *mwindow, 
	EDL *edl, 
	PluginServer *plugin_server, 
	KeyFrame *keyframe,
	int64_t start,
	int64_t end,
	File *file)
{
	this->mwindow = mwindow;
	this->edl = edl;
	this->plugin_server = plugin_server;
	this->keyframe = keyframe;
	this->start = start;
	this->end = end;
	this->file = file;

	cache = new CICache(mwindow->preferences, mwindow->plugindb);
	buffer_size = get_bufsize();
	get_recordable_tracks();
	create_modules();
	create_buffers();

	if(!plugin_server->realtime)
	{
		PluginServer *plugin;
		int i;

		if(!plugin_server->multichannel)
		{
// ============================ single channel plugins
// start 1 plugin for each track
			for(i = 0; i < total_tracks(); i++)
			{
				append(plugin = new PluginServer(*plugin_server));
				plugin->set_mwindow(mwindow);
				plugin->set_keyframe(keyframe);
				plugin->append_module(modules[i]);
				plugin->open_plugin(0, 
					mwindow->preferences, 
					mwindow->edl, 
					0,
					-1);
				if(i == 0) plugin->set_interactive();
				plugin->start_loop(start, end, buffer_size, 1);
			}
		}
		else
		{
// ============================ multichannel
// start 1 plugin for all tracks
			append(plugin = new PluginServer(*plugin_server));
			plugin->set_mwindow(mwindow);
			plugin->set_keyframe(keyframe);
			for(i = 0; i < total_tracks(); i++)
				plugin->append_module(modules[i]);
			plugin->open_plugin(0, 
				mwindow->preferences, 
				mwindow->edl, 
				0,
				-1);
// set one plugin for progress bars
			plugin->set_interactive();
			plugin->start_loop(start, end, buffer_size, total_tracks());
		}

//printf("PluginArray::start_plugins 5\n");
	}
	else
	{
		PluginServer *plugin;
		int i;

		if(!plugin_server->multichannel)
		{
// single channel plugins
// start 1 plugin for each track
			for(i = 0; i < total_tracks(); i++)
			{
				append(plugin = new PluginServer(*plugin_server));
				plugin->set_mwindow(mwindow);
				plugin->set_keyframe(keyframe);
				plugin->append_module(modules[i]);
				plugin->open_plugin(0, 
					mwindow->preferences, 
					mwindow->edl, 
					0,
					-1);
				plugin->get_parameters(start, end, 1);
				plugin->init_realtime(0, 1, get_bufsize());
			}
		}
		else
		{
// multichannel
// start 1 plugin for all tracks
			append(plugin = new PluginServer(*plugin_server));
			plugin->set_mwindow(mwindow);
			plugin->set_keyframe(keyframe);
			for(i = 0; i < total_tracks(); i++)
				plugin->append_module(modules[i]);
			plugin->open_plugin(0, 
				mwindow->preferences,
				mwindow->edl, 
				0,
				-1);
			plugin->get_parameters(start, end, total_tracks());
			plugin->init_realtime(0, total_tracks(), get_bufsize());
		}
	}
//printf("PluginArray::start_plugins 8\n");
	return 0;
}





int PluginArray::run_plugins()
{
	int i, j, result;
// Length to write after process_loop
	int64_t write_length;

	done = 0;     // for when done
	error = 0;
	if(plugin_server->realtime)
	{
		int64_t len;
		MainProgressBar *progress;
		char string[BCTEXTLEN], string2[BCTEXTLEN];

		sprintf(string, _("%s..."), plugin_server->title);
		progress = mwindow->mainprogress->start_progress(string, end - start);

		for(int current_position = start; 
			current_position < end && !done && !error;
			current_position += len)
		{
			len = buffer_size;
			if(current_position + len > end) len = end - current_position;

// Process in plugin.  This pulls data from the modules
			get_buffers();
			for(i = 0; i < total; i++)
			{
				process_realtime(i, current_position, len);
			}

// Write to file
			error = write_buffers(len);
			done = progress->update(current_position - start + len);
		}

		progress->get_time(string2);
		progress->stop_progress();
		delete progress;

		sprintf(string, _("%s took %s"), plugin_server->title, string2);
		mwindow->gui->lock_window();
		mwindow->gui->show_message(string2);
		mwindow->gui->unlock_window();
	}
	else
	{
// Run main loop once for multichannel plugins.
// Run multiple times for single channel plugins.
// Each write to the file must contain all the channels
		while(!done && !error)
		{
			for(i = 0; i < total; i++)
			{
				write_length = 0;
				done += process_loop(i, write_length);
			}


			if(write_length)
				error = write_buffers(write_length);
		}
	}

	return error;
}


int PluginArray::stop_plugins()
{
	if(plugin_server->realtime)
	{
		for(int i = 0; i < total; i++)
		{
			values[i]->close_plugin();
		}
	}
	else
	{
		for(int i = 0; i < total; i++)
		{
			values[i]->stop_loop();
			values[i]->close_plugin();
		}
	}

	delete cache;
	return 0;
}

