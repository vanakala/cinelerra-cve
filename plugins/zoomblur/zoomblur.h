
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

#ifndef ZOOMBLUR_H
#define ZOOMBLUR_H

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME
#define PLUGIN_USES_TMPFRAME

#define PLUGIN_TITLE N_("Zoom Blur")
#define PLUGIN_CLASS ZoomBlurMain
#define PLUGIN_CONFIG_CLASS ZoomBlurConfig
#define PLUGIN_THREAD_CLASS ZoomBlurThread
#define PLUGIN_GUI_CLASS ZoomBlurWindow

#include "pluginmacros.h"

#include "bcslider.h"
#include "bctoggle.h"
#include "keyframe.inc"
#include "language.h"
#include "loadbalance.h"
#include "picon_png.h"
#include "pluginvclient.h"
#include "pluginwindow.h"
#include "vframe.inc"

class ZoomBlurEngine;

class ZoomBlurConfig
{
public:
	ZoomBlurConfig();

	int equivalent(ZoomBlurConfig &that);
	void copy_from(ZoomBlurConfig &that);
	void interpolate(ZoomBlurConfig &prev, 
		ZoomBlurConfig &next, 
		ptstime prev_pts,
		ptstime next_pts,
		ptstime current_pts);

	int x;
	int y;
	int radius;
	int steps;
	int r;
	int g;
	int b;
	int a;
	PLUGIN_CONFIG_CLASS_MEMBERS
};

class ZoomBlurSize : public BC_ISlider
{
public:
	ZoomBlurSize(ZoomBlurMain *plugin, 
		int x, 
		int y, 
		int *output,
		int min,
		int max);
	int handle_event();
	ZoomBlurMain *plugin;
	int *output;
};

class ZoomBlurToggle : public BC_CheckBox
{
public:
	ZoomBlurToggle(ZoomBlurMain *plugin, 
		int x, 
		int y, 
		int *output,
		char *string);
	int handle_event();
	ZoomBlurMain *plugin;
	int *output;
};

class ZoomBlurWindow : public PluginWindow
{
public:
	ZoomBlurWindow(ZoomBlurMain *plugin, int x, int y);

	void update();

	ZoomBlurSize *x, *y, *radius, *steps;
	ZoomBlurToggle *r, *g, *b, *a;
	PLUGIN_GUI_CLASS_MEMBERS
};


PLUGIN_THREAD_HEADER


// Output coords for a layer of blurring
// Used for OpenGL only
class ZoomBlurLayer
{
public:
	ZoomBlurLayer() {};
	float x1, y1, x2, y2;
};

class ZoomBlurMain : public PluginVClient
{
public:
	ZoomBlurMain(PluginServer *server);
	~ZoomBlurMain();

	VFrame *process_tmpframe(VFrame *frame);
	void load_defaults();
	void save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void handle_opengl();

	PLUGIN_CLASS_MEMBERS

	void delete_tables();
	VFrame *input, *output;
	ZoomBlurEngine *engine;
	int **scale_y_table;
	int **scale_x_table;
	ZoomBlurLayer *layer_table;
	int table_entries;
// The accumulation buffer is needed because 8 bits isn't precise enough
	unsigned char *accum;
};

class ZoomBlurPackage : public LoadPackage
{
public:
	ZoomBlurPackage();
	int y1, y2;
};

class ZoomBlurUnit : public LoadClient
{
public:
	ZoomBlurUnit(ZoomBlurEngine *server, ZoomBlurMain *plugin);
	void process_package(LoadPackage *package);
	ZoomBlurEngine *server;
	ZoomBlurMain *plugin;
};

class ZoomBlurEngine : public LoadServer
{
public:
	ZoomBlurEngine(ZoomBlurMain *plugin, 
		int total_clients, 
		int total_packages);
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
	ZoomBlurMain *plugin;
};

#endif

