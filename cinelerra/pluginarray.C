#include "atrack.h"
#include "cache.h"
#include "edl.h"
#include "edlsession.h"
#include "file.h"
#include "keyframe.h"
#include "mainprogress.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "pluginarray.h"
#include "pluginserver.h"
#include "preferences.h"
#include "bcprogressbox.h"




PluginArray::PluginArray()
 : ArrayList<PluginServer*>()
{
}

PluginArray::~PluginArray()
{
	remove_all_objects();
	delete [] modules;
}


PluginServer* PluginArray::scan_plugindb(char *title)
{
	return mwindow->scan_plugindb(title);
}

int PluginArray::start_plugins(MWindow *mwindow, 
	EDL *edl, 
	PluginServer *plugin_server, 
	KeyFrame *keyframe,
	long start,
	long end,
	File *file)
{
	this->mwindow = mwindow;
	this->edl = edl;
	this->plugin_server = plugin_server;
	this->keyframe = keyframe;
	this->start = start;
	this->end = end;
	this->file = file;

	cache = new CICache(this->edl, mwindow->preferences, mwindow->plugindb);
//printf("PluginArray::start_plugins 1\n");	
	buffer_size = get_bufsize();
	get_recordable_tracks();
	create_modules();
	create_buffers();

//printf("PluginArray::start_plugins 2\n");
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
//printf("PluginArray::start_plugins 3 %d\n", i);
				append(plugin = new PluginServer(*plugin_server));
				plugin->set_mwindow(mwindow);
				plugin->set_keyframe(keyframe);
				plugin->set_module(modules[i]);
				plugin->open_plugin(0, mwindow->edl, 0);
				if(i == 0) plugin->set_interactive();
				plugin->start_loop(start, end, buffer_size, 1);
//printf("PluginArray::start_plugins 4\n");
			}
		}
		else
		{
// ============================ multichannel
// start 1 plugin for all tracks
//printf("PluginArray::start_plugins 5\n");
			append(plugin = new PluginServer(*plugin_server));
//printf("PluginArray::start_plugins 4\n");
			plugin->set_mwindow(mwindow);
//printf("PluginArray::start_plugins 4\n");
			plugin->set_keyframe(keyframe);
//printf("PluginArray::start_plugins 4\n");
			for(i = 0; i < total_tracks(); i++)
				plugin->set_module(modules[i]);
//printf("PluginArray::start_plugins 4\n");
			plugin->open_plugin(0, mwindow->edl, 0);
// set one plugin for progress bars
			plugin->set_interactive();
//printf("PluginArray::start_plugins 4\n");
			plugin->start_loop(start, end, buffer_size, total_tracks());
//printf("PluginArray::start_plugins 6\n");
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
				plugin->open_plugin(0, mwindow->edl, 0);
				plugin->init_realtime(0, 1, get_bufsize());
// Plugin loads configuration on its own
//			plugin->get_configuration_change(plugin_data);				
			}
		}
		else
		{
// multichannel
// start 1 plugin for all tracks
			append(plugin = new PluginServer(*plugin_server));
			plugin->set_mwindow(mwindow);
			plugin->set_keyframe(keyframe);
			plugin->open_plugin(0, mwindow->edl, 0);
			plugin->init_realtime(0, total_tracks(), get_bufsize());
// Plugin loads configuration on its own
//		plugin->get_configuration_change(plugin_data);				
		}
	}
//printf("PluginArray::start_plugins 8\n");
	return 0;
}





int PluginArray::run_plugins()
{
	int i, j, result;
// Length to write after process_loop
	long write_length;

	done = 0;     // for when done
	error = 0;
//printf("PluginArray::run_plugins 1\n");
	if(plugin_server->realtime)
	{
		long len;
		MainProgressBar *progress;
		char string[BCTEXTLEN], string2[BCTEXTLEN];

//printf("PluginArray::run_plugins 2\n");
		sprintf(string, "%s...", plugin_server->title);
		progress = mwindow->mainprogress->start_progress(string, end - start);

//printf("PluginArray::run_plugins 3\n");
		for(int current_position = start; 
			current_position < end && !done && !error;
			current_position += len)
		{
			len = buffer_size;
			if(current_position + len > end) len = end - current_position;

//printf("PluginArray::run_plugins 4\n");
// Arm buffers
			for(i = 0; i < total_tracks(); i++)
			{
				load_module(i, current_position, len);
			}
//printf("PluginArray::run_plugins 5\n");

// Process in plugin
			for(i = 0; i < total; i++)
			{
				process_realtime(i, current_position, len);
			}
//printf("PluginArray::run_plugins 6 %d\n", len);

// Write to file
			error = write_buffers(len);
			done = progress->update(current_position - start + len);
//printf("PluginArray::run_plugins 7 %d %d %d %d\n", error, done, current_position, end);
		}

		progress->get_time(string2);
		progress->stop_progress();
		delete progress;

		sprintf(string, "%s took %s", plugin_server->title, string2);
		mwindow->gui->lock_window();
		mwindow->gui->show_message(string2, BLACK);
		mwindow->gui->unlock_window();
//printf("PluginArray::run_plugins 8\n");
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
//printf("PluginArray::run_plugins 9 %d\n", i);
				write_length = 0;
				done += process_loop(i, write_length);
//printf("PluginArray::run_plugins 10 %d\n", write_length);
			}


			if(write_length)
				error = write_buffers(write_length);
//printf("PluginArray::run_plugins 11 %d\n", write_length);
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

