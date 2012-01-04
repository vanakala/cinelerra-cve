
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

#ifndef INTERPOLATEPIXELS_H
#define INTERPOLATEPIXELS_H

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME

#define PLUGIN_TITLE N_("Interpolate Pixels");
#define PLUGIN_CLASS InterpolatePixelsMain
#define PLUGIN_CONFIG_CLASS InterpolatePixelsConfig
#define PLUGIN_THREAD_CLASS InterpolatePixelsThread
#define PLUGIN_GUI_CLASS InterpolatePixelsWindow

#include "pluginmacros.h"

class InterpolatePixelsMain;

#define TOTAL_PATTERNS 2

#include "bchash.h"
#include "language.h"
#include "loadbalance.h"
#include "pluginvclient.h"
#include <sys/types.h>

class InterpolatePixelsConfig
{
public:
	InterpolatePixelsConfig();

	int equivalent(InterpolatePixelsConfig &that);
	void copy_from(InterpolatePixelsConfig &that);
	void interpolate(InterpolatePixelsConfig &prev,
	InterpolatePixelsConfig &next,
		ptstime prev_position,
		ptstime next_position,
		ptstime current_position);
	int x, y;
	PLUGIN_CONFIG_CLASS_MEMBERS
};

class InterpolatePixelsEngine;

class InterpolatePixelsOffset : public BC_ISlider
{
public:
	InterpolatePixelsOffset(InterpolatePixelsWindow *window, 
		int x, 
		int y, 
		int *output);
	~InterpolatePixelsOffset();

	int handle_event();
	InterpolatePixelsWindow *window;
	int *output;
};

class InterpolatePixelsWindow : public BC_Window
{
public:
	InterpolatePixelsWindow(InterpolatePixelsMain *plugin, int x, int y);
	~InterpolatePixelsWindow();

	void update();

	InterpolatePixelsOffset *x_offset;
	InterpolatePixelsOffset *y_offset;
	PLUGIN_GUI_CLASS_MEMBERS
};

PLUGIN_THREAD_HEADER

class InterpolatePixelsMain : public PluginVClient
{
public:
	InterpolatePixelsMain(PluginServer *server);
	~InterpolatePixelsMain();

	PLUGIN_CLASS_MEMBERS

	void process_frame(VFrame *frame);

	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void load_defaults();
	void save_defaults();
	void handle_opengl();

	InterpolatePixelsEngine *engine;
};


class InterpolatePixelsPackage : public LoadPackage
{
public:
	InterpolatePixelsPackage();
	int y1, y2;
};

class InterpolatePixelsUnit : public LoadClient
{
public:
	InterpolatePixelsUnit(InterpolatePixelsEngine *server, InterpolatePixelsMain *plugin);
	void process_package(LoadPackage *package);
	InterpolatePixelsEngine *server;
	InterpolatePixelsMain *plugin;
};

class InterpolatePixelsEngine : public LoadServer
{
public:
	InterpolatePixelsEngine(InterpolatePixelsMain *plugin);
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
	InterpolatePixelsMain *plugin;
	float color_matrix[9];
};


#endif
