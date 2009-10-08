
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

#ifndef DOT_H
#define DOT_H

class DotMain;
class DotEngine;

#include "bchash.h"
#include "effecttv.h"
#include "loadbalance.h"
#include "mutex.h"
#include "pluginvclient.h"
#include "dotwindow.h"

#include <stdint.h>


class DotConfig
{
public:
	DotConfig();


	int dot_depth;
	int dot_size;
	inline int dot_max()
	{
		return 1 << dot_depth;
	};
};

class DotPackage : public LoadPackage
{
public:
	DotPackage();

	int row1, row2;
};

class DotServer : public LoadServer
{
public:
	DotServer(DotMain *plugin, int total_clients, int total_packages);
	
	LoadClient* new_client();
	LoadPackage* new_package();
	void init_packages();
	DotMain *plugin;
};

class DotClient : public LoadClient
{
public:
	DotClient(DotServer *server);
	
	void process_package(LoadPackage *package);
	void draw_dot(int xx, 
		int yy, 
		unsigned char c, 
		unsigned char **output_rows,
		int color_model);

	unsigned char RGBtoY(unsigned char *row, int color_model);

	DotMain *plugin;
};


class DotMain : public PluginVClient
{
public:
	DotMain(PluginServer *server);
	~DotMain();

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
	void make_pattern();
	void init_sampxy_table();
	void reconfigure();

// a thread for the GUI
	DotThread *thread;
	DotServer *dot_server;
	DotClient *dot_client;
	DotConfig config;

	BC_Hash *defaults;
	VFrame *input_ptr, *output_ptr;
	int dots_width;
	int dots_height;
	int dot_size;
	int dot_hsize;
	uint32_t *pattern;
	int *sampx, *sampy;
	int need_reconfigure;
	EffectTV *effecttv;
};










#endif
