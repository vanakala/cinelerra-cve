
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

#ifndef AGING_H
#define AGING_H

class AgingMain;
class AgingEngine;

#include "bchash.h"
#include "loadbalance.h"
#include "mutex.h"
#include "pluginvclient.h"
#include "agingwindow.h"
#include <sys/types.h>

#define SCRATCH_MAX 20


typedef struct _scratch
{
	int life;
	int x;
	int dx;
	int init;
} scratch_t;

class AgingConfig
{
public:
	AgingConfig();

	int area_scale;
	int aging_mode;
	scratch_t scratches[SCRATCH_MAX];

	static int dx[8];
	static int dy[8];
	int dust_interval;


	int pits_interval;
	
	int scratch_lines;
	int pit_count;
	int dust_count;
	
	int colorage;
	int scratch;
	int pits;
	int dust;
};

class AgingPackage : public LoadPackage
{
public:
	AgingPackage();

	int row1, row2;
};

class AgingServer : public LoadServer
{
public:
	AgingServer(AgingMain *plugin, int total_clients, int total_packages);
	
	LoadClient* new_client();
	LoadPackage* new_package();
	void init_packages();
	AgingMain *plugin;
};

class AgingClient : public LoadClient
{
public:
	AgingClient(AgingServer *server);
	
	void coloraging(unsigned char **output_ptr, 
		unsigned char **input_ptr,
		int color_model,
		int w,
		int h);
	void scratching(unsigned char **output_ptr,
		int color_model,
		int w,
		int h);
	void pits(unsigned char **output_ptr,
		int color_model,
		int w,
		int h);
	void dusts(unsigned char **output_ptr,
		int color_model,
		int w,
		int h);
	void process_package(LoadPackage *package);

	AgingMain *plugin;
};


class AgingMain : public PluginVClient
{
public:
	AgingMain(PluginServer *server);
	~AgingMain();

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

// a thread for the GUI
	AgingThread *thread;
	AgingServer *aging_server;
	AgingClient *aging_client;
	AgingConfig config;

	BC_Hash *defaults;
	AgingEngine **engine;
	VFrame *input_ptr, *output_ptr;
};










#endif
