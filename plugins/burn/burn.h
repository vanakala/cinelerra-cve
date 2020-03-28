
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

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME
#define PLUGIN_USES_TMPFRAME
#define PLUGIN_CUSTOM_LOAD_CONFIGURATION

#define PLUGIN_TITLE N_("BurningTV")
#define PLUGIN_CLASS BurnMain
#define PLUGIN_CONFIG_CLASS BurnConfig
#define PLUGIN_THREAD_CLASS BurnThread
#define PLUGIN_GUI_CLASS BurnWindow

#include "pluginmacros.h"

#include "effecttv.inc"
#include "language.h"
#include "loadbalance.h"
#include "pluginvclient.h"
#include "burnwindow.h"

class BurnConfig
{
public:
	BurnConfig();
	int threshold;
	int decay;
	double recycle;  // Seconds to a recycle
	PLUGIN_CONFIG_CLASS_MEMBERS
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

	PLUGIN_CLASS_MEMBERS

// required for all realtime plugins
	VFrame *process_tmpframe(VFrame *input);

	void HSItoRGB(double H, 
		double S, 
		double I, 
		int *r, 
		int *g, 
		int *b,
		int color_model);
	void make_palette(int color_model);

// a thread for the GUI
	BurnServer *burn_server;

	int palette[3][256];
	unsigned char *buffer;

	int total;

	EffectTV *effecttv;
	VFrame *input_ptr, *output_ptr;
};

#endif
