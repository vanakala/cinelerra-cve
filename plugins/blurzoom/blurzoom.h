
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

#ifndef BLURZOOM_H
#define BLURZOOM_H

class BlurZoomMain;
class BlurZoomEngine;

#include "bchash.h"
#include "loadbalance.h"
#include "mutex.h"
#include "pluginvclient.h"
#include "blurzoomwindow.h"
#include <sys/types.h>

#define SCRATCH_MAX 20


typedef struct _scratch
{
	int life;
	int x;
	int dx;
	int init;
} scratch_t;

class BlurZoomConfig
{
public:
	BlurZoomConfig();

};

class BlurZoomPackage : public LoadPackage
{
public:
	BlurZoomPackage();

	int row1, row2;
};

class BlurZoomServer : public LoadServer
{
public:
	BlurZoomServer(BlurZoomMain *plugin, int total_clients, int total_packages);
	
	LoadClient* new_client();
	LoadPackage* new_package();
	void init_packages();
	BlurZoomMain *plugin;
};

class BlurZoomClient : public LoadClient
{
public:
	BlurZoomClient(BlurZoomServer *server);
	
	void process_package(LoadPackage *package);

	BlurZoomMain *plugin;
};


class BlurZoomMain : public PluginVClient
{
public:
	BlurZoomMain(PluginServer *server);
	~BlurZoomMain();

// required for all realtime plugins
	int process_realtime(VFrame *input_ptr, VFrame *output_ptr);
	int is_realtime();
	char* plugin_title();
	int start_realtime();
	int stop_realtime();
	int show_gui();
	void raise_window();
	int set_string();
	void load_configuration();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	
	int load_defaults();
	int save_defaults();
	VFrame* new_picon();

// a thread for the GUI
	BlurZoomThread *thread;
	BlurZoomServer *blurzoom_server;
	BlurZoomClient *blurzoom_client;
	BlurZoomConfig config;

	unsigned char *blurzoombuf;
	int *blurzoomx;
	int *blurzoomy;
	int buf_width_blocks;
	int buf_width;
	int buf_height;
	int buf_area;
	int buf_margin_right;
	int buf_margin_left;

#define COLORS 32
	int palette_r[COLORS];
	int palette_g[COLORS];
	int palette_b[COLORS];

	int y_threshold;

	BC_Hash *defaults;
	BlurZoomEngine **engine;
	VFrame *input_ptr, *output_ptr;
};










#endif
