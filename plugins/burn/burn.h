
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

#ifndef BURN_H
#define BURN_H

class BurnMain;

#include "bchash.h"
#include "effecttv.inc"
#include "loadbalance.h"
#include "mutex.h"
#include "plugincolors.inc"
#include "pluginvclient.h"
#include "burnwindow.h"
#include <sys/types.h>

class BurnConfig
{
public:
	BurnConfig();
	int threshold;
	int decay;
	double recycle;  // Seconds to a recycle
};

class BurnPackage : public LoadPackage
{
public:
	BurnPackage();

	int row1, row2;
};

class BurnServer : public LoadServer
{
public:
	BurnServer(BurnMain *plugin, int total_clients, int total_packages);
	
	LoadClient* new_client();
	LoadPackage* new_package();
	void init_packages();
	BurnMain *plugin;
};

class BurnClient : public LoadClient
{
public:
	BurnClient(BurnServer *server);
	
	void process_package(LoadPackage *package);

	BurnMain *plugin;
};


class BurnMain : public PluginVClient
{
public:
	BurnMain(PluginServer *server);
	~BurnMain();

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




	void HSItoRGB(double H, 
		double S, 
		double I, 
		int *r, 
		int *g, 
		int *b,
		int color_model);
	void make_palette(int color_model);

// a thread for the GUI
	BurnThread *thread;
	BurnServer *burn_server;
	BurnConfig config;

	int palette[3][256];
	unsigned char *buffer;
	
	int total;

	EffectTV *effecttv;
	BC_Hash *defaults;
	VFrame *input_ptr, *output_ptr;
	YUV *yuv;
};










#endif
