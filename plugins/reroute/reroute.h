
/*
 * CINELERRA
 * Copyright (C) 2007 Hermann Vosseler
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

#ifndef REROUTE_H
#define REROUTE_H

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME
#define PLUGIN_IS_MULTICHANNEL
#define PLUGIN_CUSTOM_LOAD_CONFIGURATION

#define PLUGIN_TITLE N_("Reroute")
#define PLUGIN_CLASS Reroute
#define PLUGIN_CONFIG_CLASS RerouteConfig
#define PLUGIN_THREAD_CLASS RerouteThread
#define PLUGIN_GUI_CLASS RerouteWindow

#include "pluginmacros.h"

#include "keyframe.inc"
#include "pluginserver.inc"
#include "pluginwindow.h"
#include "pluginvclient.h"
#include "vframe.inc"

class Reroute;

class RerouteConfig
{
public:
	RerouteConfig();

	static const char* operation_to_text(int operation);
	int operation;
	enum
	{
		REPLACE,
		REPLACE_COMPONENTS,
		REPLACE_ALPHA
	};

	static const char* output_to_text(int output_track);
	int output_track;
	enum
	{
		TOP,
		BOTTOM
	};
	PLUGIN_CONFIG_CLASS_MEMBERS
};


class RerouteOperation : public BC_PopupMenu
{
public:
	RerouteOperation(Reroute *plugin,
		int x, 
		int y);

	void create_objects();
	int handle_event();
	Reroute *plugin;
};

class RerouteOutput : public BC_PopupMenu
{
public:
	RerouteOutput(Reroute *plugin,
		int x, 
		int y);

	void create_objects();
	int handle_event();
	Reroute *plugin;
};


class RerouteWindow : public PluginWindow
{
public:
	RerouteWindow(Reroute *plugin, int x, int y);

	void update();

	RerouteOperation *operation;
	RerouteOutput *output;
	PLUGIN_GUI_CLASS_MEMBERS
};

PLUGIN_THREAD_HEADER

class Reroute : public PluginVClient
{
public:
	Reroute(PluginServer *server);
	~Reroute();

	PLUGIN_CLASS_MEMBERS

	void process_frame(VFrame **frame);
	void load_defaults();
	void save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);

	int output_track;
	int input_track;
};

#endif
