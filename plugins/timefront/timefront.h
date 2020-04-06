
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

#ifndef TIMEFRONT_H
#define TIMEFRONT_H

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME
#define PLUGIN_IS_SYNTHESIS
#define PLUGIN_IS_MULTICHANNEL
#define PLUGIN_USES_TMPFRAME
#define PLUGIN_MAX_CHANNELS 2

#define PLUGIN_TITLE N_("TimeFront")
#define PLUGIN_CLASS TimeFrontMain
#define PLUGIN_CONFIG_CLASS TimeFrontConfig
#define PLUGIN_THREAD_CLASS TimeFrontThread
#define PLUGIN_GUI_CLASS TimeFrontWindow

#include "pluginmacros.h"

class TimeFrontEngine;
class TimeFrontServer;

#include "bchash.inc"
#include "bcpopupmenu.h"
#include "bcpot.h"
#include "bcslider.h"
#include "language.h"
#include "loadbalance.h"
#include "overlayframe.inc"
#include "pluginvclient.h"
#include "thread.h"
#include "vframe.inc"
#include "pluginwindow.h"

#define MAX_TIME_RANGE 10
#define MAX_FRAMELIST (100 * MAX_TIME_RANGE)

class TimeFrontConfig
{
public:
	TimeFrontConfig();

	int equivalent(TimeFrontConfig &that);
	void copy_from(TimeFrontConfig &that);
	void interpolate(TimeFrontConfig &prev, 
		TimeFrontConfig &next, 
		ptstime prev_pts,
		ptstime next_pts,
		ptstime current_pts);
// Int to hex triplet conversion
	int get_in_color();
	int get_out_color();

// LINEAR or RADIAL
	int shape;
// LINEAR or LOG or SQUARE
	int rate;
	enum
	{
		LINEAR,
		RADIAL,
		LOG,
		SQUARE,
		OTHERTRACK,
		ALPHA
	};

	double center_x;
	double center_y;
	double angle;
	double in_radius;
	double out_radius;
	ptstime time_range;
	int track_usage;
	enum 
	{
		OTHERTRACK_INTENSITY,
		OTHERTRACK_ALPHA,
	};
	int invert;
	int show_grayscale;
	PLUGIN_CONFIG_CLASS_MEMBERS
};


class TimeFrontShape : public BC_PopupMenu
{
public:
	TimeFrontShape(TimeFrontMain *plugin, 
		TimeFrontWindow *gui,
		int x, 
		int y);

	static const char* to_text(int shape);
	static int from_text(const char *text);
	int handle_event();
	TimeFrontMain *plugin;
	TimeFrontWindow *gui;
};

class TimeFrontTrackUsage : public BC_PopupMenu
{
public:
	TimeFrontTrackUsage(TimeFrontMain *plugin, 
		TimeFrontWindow *gui,
		int x, 
		int y);

	static const char* to_text(int track_usage);
	static int from_text(const char *text);
	int handle_event();
	TimeFrontMain *plugin;
	TimeFrontWindow *gui;
};


class TimeFrontRate : public BC_PopupMenu
{
public:
	TimeFrontRate(TimeFrontMain *plugin, 
		int x, 
		int y);
	void create_objects();
	static const char* to_text(int shape);
	static int from_text(char *text);
	int handle_event();
	TimeFrontMain *plugin;
	TimeFrontWindow *gui;
};

class TimeFrontCenterX : public BC_FPot
{
public:
	TimeFrontCenterX(TimeFrontMain *plugin, int x, int y);
	int handle_event();
	TimeFrontMain *plugin;
};

class TimeFrontCenterY : public BC_FPot
{
public:
	TimeFrontCenterY(TimeFrontMain *plugin, int x, int y);
	int handle_event();
	TimeFrontMain *plugin;
};

class TimeFrontAngle : public BC_FPot
{
public:
	TimeFrontAngle(TimeFrontMain *plugin, int x, int y);
	int handle_event();
	TimeFrontMain *plugin;
};

class TimeFrontInRadius : public BC_FSlider
{
public:
	TimeFrontInRadius(TimeFrontMain *plugin, int x, int y);
	int handle_event();
	TimeFrontMain *plugin;
};

class TimeFrontOutRadius : public BC_FSlider
{
public:
	TimeFrontOutRadius(TimeFrontMain *plugin, int x, int y);
	int handle_event();
	TimeFrontMain *plugin;
};

class TimeFrontFrameRange : public BC_FSlider
{
public:
	TimeFrontFrameRange(TimeFrontMain *plugin, int x, int y);
	int handle_event();
	TimeFrontMain *plugin;
};


class TimeFrontInvert : public BC_CheckBox
{
public:
	TimeFrontInvert(TimeFrontMain *plugin, int x, int y);
	int handle_event();
	TimeFrontMain *plugin;
};

class TimeFrontShowGrayscale : public BC_CheckBox
{
public:
	TimeFrontShowGrayscale(TimeFrontMain *plugin, int x, int y);
	int handle_event();
	TimeFrontMain *plugin;
};


class TimeFrontWindow : public PluginWindow
{
public:
	TimeFrontWindow(TimeFrontMain *plugin, int x, int y);

	void update();
	void update_shape();

	BC_Title *angle_title;
	BC_Title *rate_title, *in_radius_title, *out_radius_title, *track_usage_title;
	TimeFrontAngle *angle;
	TimeFrontInRadius *in_radius;
	TimeFrontOutRadius *out_radius;
	TimeFrontFrameRange *frame_range;
	TimeFrontShape *shape;
	TimeFrontTrackUsage *track_usage;
	BC_Title *shape_title;
	TimeFrontCenterX *center_x;
	BC_Title *center_x_title;
	BC_Title *center_y_title;
	TimeFrontCenterY *center_y;
	TimeFrontRate *rate;
	TimeFrontShowGrayscale *show_grayscale;
	TimeFrontInvert *invert;
	int frame_range_x, frame_range_y;
	int shape_x, shape_y;
	PLUGIN_GUI_CLASS_MEMBERS
};



PLUGIN_THREAD_HEADER


class TimeFrontMain : public PluginVClient
{
public:
	TimeFrontMain(PluginServer *server);
	~TimeFrontMain();

	void process_tmpframes(VFrame **frame);

	void load_defaults();
	void save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);

	PLUGIN_CLASS_MEMBERS

	OverlayFrame *overlayer;
	VFrame *gradient;
	VFrame *input;
	TimeFrontServer *engine;
	int framelist_allocated;
	int framelist_last;
	VFrame *framelist[MAX_FRAMELIST];
};

class TimeFrontPackage : public LoadPackage
{
public:
	TimeFrontPackage();
	int y1;
	int y2;
};

class TimeFrontUnit : public LoadClient
{
public:
	TimeFrontUnit(TimeFrontServer *server, TimeFrontMain *plugin);
	void process_package(LoadPackage *package);
	TimeFrontServer *server;
	TimeFrontMain *plugin;
};

class TimeFrontServer : public LoadServer
{
public:
	TimeFrontServer(TimeFrontMain *plugin, int total_clients, int total_packages);
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
	TimeFrontMain *plugin;
};

#endif
