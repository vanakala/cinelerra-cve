
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

class HoloMain;
class HoloEngine;

#include "bchash.h"
#include "effecttv.h"
#include "holowindow.h"
#include "loadbalance.h"
#include "mutex.h"
#include "plugincolors.inc"
#include "pluginvclient.h"

#include <stdint.h>


class HoloConfig
{
public:
	HoloConfig();


	int threshold;
	double recycle;    // Number of seconds between recycles
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

// required for all realtime plugins
	int process_realtime(VFrame *input_ptr, VFrame *output_ptr);
	int is_realtime();
	char* plugin_title();
	int show_gui();
	void raise_window();
	int set_string();
	void load_configuration();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);

	int load_defaults();
	int save_defaults();
	VFrame* new_picon();
	void reconfigure();


	void add_frames(VFrame *output, VFrame *input);
	void set_background();

// a thread for the GUI
	HoloThread *thread;
	HoloServer *holo_server;
	HoloConfig config;

	BC_Hash *defaults;
	VFrame *input_ptr, *output_ptr;
	int do_reconfigure;
	EffectTV *effecttv;

	unsigned int noisepattern[256];
	VFrame *bgimage, *tmp;
	YUV *yuv;
	int total;
};










#endif
