
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

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME
#define PLUGIN_CUSTOM_LOAD_CONFIGURATION

#define PLUGIN_TITLE N_("DotTV")
#define PLUGIN_CLASS DotMain
#define PLUGIN_CONFIG_CLASS DotConfig
#define PLUGIN_THREAD_CLASS DotThread
#define PLUGIN_GUI_CLASS DotWindow

#include "pluginmacros.h"

class DotEngine;

#include "bchash.h"
#include "effecttv.h"
#include "loadbalance.h"
#include "pluginvclient.h"
#include "dotwindow.h"
#include "language.h"

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
	PLUGIN_CONFIG_CLASS_MEMBERS
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
		unsigned char *output_row,
		int color_model);

	unsigned char RGBtoY(unsigned char *row, int color_model);

	DotMain *plugin;
};


class DotMain : public PluginVClient
{
public:
	DotMain(PluginServer *server);
	~DotMain();

	PLUGIN_CLASS_MEMBERS

// required for all realtime plugins
	void process_realtime(VFrame *input_ptr, VFrame *output_ptr);

	void make_pattern();
	void init_sampxy_table();
	void reconfigure();

	DotServer *dot_server;
	DotClient *dot_client;

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
