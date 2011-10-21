
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

#ifndef HOLO_H
#define HOLO_H

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME
#define PLUGIN_CUSTOM_LOAD_CONFIGURATION

#define PLUGIN_TITLE N_("HolographicTV")
#define PLUGIN_CLASS HoloMain
#define PLUGIN_CONFIG_CLASS HoloConfig
#define PLUGIN_THREAD_CLASS HoloThread
#define PLUGIN_GUI_CLASS HoloWindow

#include "pluginmacros.h"

class HoloEngine;

#include "bchash.h"
#include "effecttv.h"
#include "language.h"
#include "holowindow.h"
#include "loadbalance.h"
#include "plugincolors.inc"
#include "pluginvclient.h"

#include <stdint.h>

class HoloConfig
{
public:
	HoloConfig();

	int threshold;
	double recycle;    // Number of seconds between recycles
	PLUGIN_CONFIG_CLASS_MEMBERS
};

class HoloPackage : public LoadPackage
{
public:
	HoloPackage();

	int row1, row2;
};

class HoloServer : public LoadServer
{
public:
	HoloServer(HoloMain *plugin, int total_clients, int total_packages);

	LoadClient* new_client();
	LoadPackage* new_package();
	void init_packages();
	HoloMain *plugin;
};

class HoloClient : public LoadClient
{
public:
	HoloClient(HoloServer *server);
	void process_package(LoadPackage *package);

	HoloMain *plugin;
	int phase;
};


class HoloMain : public PluginVClient
{
public:
	HoloMain(PluginServer *server);
	~HoloMain();

	PLUGIN_CLASS_MEMBERS

// required for all realtime plugins
	void process_realtime(VFrame *input_ptr, VFrame *output_ptr);

	void reconfigure();

	void add_frames(VFrame *output, VFrame *input);
	void set_background();

	HoloServer *holo_server;

	VFrame *input_ptr, *output_ptr;
	int do_reconfigure;
	EffectTV *effecttv;

	unsigned int noisepattern[256];
	VFrame *bgimage, *tmp;
	YUV *yuv;
	int total;
};

#endif
