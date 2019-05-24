
/*
 * CINELERRA
 * Copyright (C) 2019 Einar Rünkaru <einarrunkaru@gmail dot com>
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

#ifndef PLUGINDB_H
#define PLUGINDB_H

#include "arraylist.h"
#include "bcprogress.h"
#include "filesystem.inc"
#include "plugindb.inc"
#include "pluginserver.inc"
#include "splashgui.inc"

extern PluginDB plugindb;

class PluginDB : ArrayList<PluginServer*>
{
public:
	PluginDB();
	~PluginDB();

	void init_plugin_path(FileSystem *fs,
		SplashGUI *splash_window,
		int *counter);
	void init_plugins(SplashGUI *splash_window);
	void fill_plugindb(int do_audio,
		int do_video,
		int is_realtime,
		int is_multichannel,
		int is_transition,
		int is_theme,
		ArrayList<PluginServer*> &plugindb);
	PluginServer *get_pluginserver(const char *title, int data_type);
	PluginServer *get_theme(const char *title);
	const char *translate_pluginname(const char *oldname, int data_type);
	int count();

	void dump(int indent = 0, int data_type = 0);
};

#endif
