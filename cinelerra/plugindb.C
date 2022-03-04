// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
// Copyright (C) 2019 Einar Rünkaru <einarrunkaru@gmail dot com>

#include "arraylist.h"
#include "bcsignals.h"
#include "filesystem.h"
#include "language.h"
#include "mainerror.h"
#include "plugindb.h"
#include "pluginserver.h"
#include "splashgui.h"
#include "preferences.h"

#include <string.h>

static struct oldpluginnames
{
	int datatype;
	char oldname[64];
	char newname[64];
} oldpluginnames[] = {
	{ STRDSC_AUDIO, "Invert Audio", "Invert" },
	{ STRDSC_AUDIO, "Delay audio", "Delay" },
	{ STRDSC_AUDIO, "Loop audio", "Loop" },
	{ STRDSC_AUDIO, "Reverse audio", "Reverse" },
	{ STRDSC_AUDIO, "Heroine College Concert Hall", "HC Concert Hall" },
	{ STRDSC_VIDEO, "Invert Video", "Invert" },
	{ STRDSC_VIDEO, "Denoise video", "Denoise" },
	{ STRDSC_VIDEO, "Selective Temporal Averaging", "STA" },
	{ STRDSC_VIDEO, "Delay Video", "Delay" },
	{ STRDSC_VIDEO, "Loop video", "Loop" },
	{ STRDSC_VIDEO, "Reverse video", "Reverse" },
	{ 0, "", "" }
};

PluginDB::PluginDB()
 : ArrayList<PluginServer*>()
{
}

PluginDB::~PluginDB()
{
	for(int i = 0; i < total; i++)
		delete values[i];
}

void PluginDB::init_plugin_path(FileSystem *fs,
	SplashGUI *splash_window,
	int *counter)
{
	int result = 0;
	PluginServer *newplugin;
 
	if(!result)
	{
		for(int i = 0; i < fs->dir_list.total; i++)
		{
			char path[BCTEXTLEN];
			strcpy(path, fs->dir_list.values[i]->path);
 
// File is a directory
			if(fs->is_dir(path))
				continue;
			else
			{
// Try to query the plugin
				fs->complete_path(path);
				PluginServer *new_plugin = new PluginServer(path);
				PluginClient *client = new_plugin->open_plugin(0, 0, 1);

				if(client)
				{
					append(new_plugin);
					new_plugin->close_plugin(client);
					new_plugin->release_plugin();
					if(splash_window)
						splash_window->operation->update(_(new_plugin->title));
				}
				else
				{
					delete new_plugin;
// Open LAD subplugins
					int id = 0;
					do
					{
						new_plugin = new PluginServer(path);
						client = new_plugin->open_plugin(0, 0,
							1, id);
						id++;
						if(client)
						{
							append(new_plugin);
							new_plugin->close_plugin(client);
							new_plugin->release_plugin();
							if(splash_window)
								splash_window->operation->update(_(new_plugin->title));
						}
						else
							delete new_plugin;
					}while(client);
				}
			}
			if(splash_window)
				splash_window->progress->update((*counter)++);
		}
	}
}

void PluginDB::init_plugins(SplashGUI *splash_window)
{
	FileSystem cinelerra_fs;
	ArrayList<FileSystem*> lad_fs;
	int result = 0;

// Get directories
	cinelerra_fs.set_filter("[*.so]");
	result = cinelerra_fs.update(preferences_global->global_plugin_dir);

	if(result)
		errorbox(_("Couldn't open plugins directory '%s'"),
			preferences_global->global_plugin_dir);

#ifdef HAVE_LADSPA
// Parse LAD environment variable
	char *env = getenv("LADSPA_PATH");

	if(env)
	{
		char string[BCTEXTLEN];
		char *ptr1 = env;

		while(ptr1)
		{
			char *ptr = strchr(ptr1, ':');
			char *end;

			if(ptr)
				end = ptr;
			else
				end = env + strlen(env);

			if(end > ptr1)
			{
				int len = end - ptr1;
				memcpy(string, ptr1, len);
				string[len] = 0;

				FileSystem *fs = new FileSystem;
				lad_fs.append(fs);
				fs->set_filter("*.so");
				result = fs->update(string);

				if(result)
					errorbox(_("Couldn't open LADSPA directory '%s'"),
						string);
			}

			if(ptr)
				ptr1 = ptr + 1;
			else
				ptr1 = ptr;
		}
	}
#endif
	int total = cinelerra_fs.total_files();
	int counter = 0;
	for(int i = 0; i < lad_fs.total; i++)
	total += lad_fs.values[i]->total_files();
	if(splash_window)
		splash_window->progress->update_length(total);

	init_plugin_path(&cinelerra_fs,
		splash_window,
		&counter);

// LAD
	for(int i = 0; i < lad_fs.total; i++)
		init_plugin_path(lad_fs.values[i],
			splash_window,
			&counter);

	lad_fs.remove_all_objects();
}

void PluginDB::fill_plugindb(int strdsc,
	int is_realtime,
	int is_transition,
	int is_theme,
	ArrayList<PluginServer*> &plugindb)
{
// Get plugins
	for(int i = 0; i < total; i++)
	{
		PluginServer *current = values[i];

		if(((((strdsc & STRDSC_AUDIO) && current->audio) ||
				((strdsc & STRDSC_VIDEO) && current->video)) &&
				(((current->realtime && is_realtime > 0) || is_realtime < 0) ||
				(current->transition && is_transition))) ||
				(current->theme && is_theme))
			plugindb.append(current);
	}
// Alphabetize list by title
	int done = 0;

	while(!done)
	{
		done = 1;

		for(int i = 0; i < plugindb.total - 1; i++)
		{
			PluginServer *value1 = plugindb.values[i];
			PluginServer *value2 = plugindb.values[i + 1];
			if(strcmp(_(value1->title), _(value2->title)) > 0)
			{
				done = 0;
				plugindb.values[i] = value2;
				plugindb.values[i + 1] = value1;
			}
		}
	}
}

PluginServer* PluginDB::get_pluginserver(int strdsc, const char *title,
	int transition)
{
	const char *new_title;

	for(int i = 0; i < total; i++)
	{
		PluginServer *server = values[i];

		if((((strdsc & STRDSC_AUDIO) && server->audio) ||
				((strdsc & STRDSC_VIDEO) && server->video)) &&
				((transition && server->transition) ||
				(!transition && !server->transition)) &&
				!strcmp(server->title, title))
			return server;
	}
// Check if name has been changed
	if(new_title = translate_pluginname(title, strdsc))
	{
		for(int i = 0; i < total; i++)
		{
			PluginServer *server = values[i];

			if((((strdsc & STRDSC_AUDIO) && server->audio) ||
					((strdsc & STRDSC_VIDEO) && server->video)) &&
					((transition && server->transition) ||
					(!transition && !server->transition)) &&
					!strcmp(server->title, new_title))
				return server;
		}
	}
	errormsg("The effect '%s' is not part of your installation of " PROGRAM_NAME ".", title);
	return 0;
}

PluginServer *PluginDB::get_theme(const char *title)
{
	for(int i = 0; i < total; i++)
	{
		PluginServer *server = values[i];

		if(server->theme && !strcmp(server->title, title))
			return server;
	}
	return 0;
}

const char *PluginDB::translate_pluginname(const char *oldname, int strdsc)
{
	struct oldpluginnames *pnp;

	for(int i = 0; oldpluginnames[i].oldname[0]; i++)
	{
		if(strdsc == oldpluginnames[i].datatype &&
				strcmp(oldpluginnames[i].oldname, oldname) == 0)
			return oldpluginnames[i].newname;
	}
	return 0;
}

int PluginDB::count()
{
	return total;
}

void PluginDB::dump(int indent, int strdsc)
{
	int typ;
	char max[4];

	printf("%*sKnown plugins:\n", indent, "");
	indent += 2;
	for(int i = 0; i < total; i++)
	{
		PluginServer *server = values[i];

		if(strdsc)
		{
			if((strdsc & STRDSC_AUDIO) && !server->audio)
				continue;
			if((strdsc & STRDSC_VIDEO) && !server->video)
				continue;
		}
		if(server->audio)
			typ = 'A';
		else if(server->video)
			typ = 'V';
		else if(server->theme)
			typ = 'T';
		else
			typ = '-';
		if(server->multichannel_max)
			sprintf(max, "%02d", server->multichannel_max);
		else
			strcpy(max, "--");
		printf("%*s%c%c%c%c%c%c%02d%c%c%c%s %s\n", indent, "",
			server->realtime ? 'R' : '-', typ,
			server->uses_gui ? 'G' : '-', server->multichannel ? 'M' : '-',
			server->synthesis ? 'S' : '-', server->transition ? 'T' : '-',
			server->apiversion, server->opengl_plugin ? 'L' : '-',
			server->status_gui ? 'U' : '-',
			server->no_keyframes ? 'K' : '-',
			max, server->title);
	}
	putchar('\n');
	printf("%*sLegend:\n", indent, "");
	indent += 2;
	printf("%*sR     - realtime\n", indent, "");
	printf("%*sA/V/T - audio/video/theme\n", indent, "");
	printf("%*sG     - has gui\n", indent, "");
	printf("%*sM     - multichannel\n", indent, "");
	printf("%*sS     - synthesis\n", indent, "");
	printf("%*sT     - transition\n", indent, "");
	printf("%*s##    - api version\n", indent, "");
	printf("%*sL     - opengl support\n", indent, "");
	printf("%*sU     - plugin has status gui\n", indent, "");
	printf("%*sK     - plugin is not keyframeable\n", indent, "");
	printf("%*s##    - number of channels\n", indent, "");
}
