
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

class TimeFrontMain;
class TimeFrontEngine;
class TimeFrontThread;
class TimeFrontWindow;
class TimeFrontServer;


#include "bchash.inc"
#include "filexml.inc"
#include "guicast.h"
#include "loadbalance.h"
#include "overlayframe.inc"
#include "plugincolors.h"
#include "pluginvclient.h"
#include "thread.h"
#include "vframe.inc"

class TimeFrontConfig
{
public:
	TimeFrontConfig();

	int equivalent(TimeFrontConfig &that);
	void copy_from(TimeFrontConfig &that);
	void interpolate(TimeFrontConfig &prev, 
		TimeFrontConfig &next, 
		long prev_frame, 
		long next_frame, 
		long current_frame);
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
	int frame_range;
	int track_usage;
	enum 
	{
		OTHERTRACK_INTENSITY,
		OTHERTRACK_ALPHA,
	};
	int invert;
	int show_grayscale;
};


class TimeFrontShape : public BC_PopupMenu
{
public:
	TimeFrontShape(TimeFrontMain *plugin, 
		TimeFrontWindow *gui,
		int x, 
		int y);
	void create_objects();
	static char* to_text(int shape);
	static int from_text(char *text);
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
	void create_objects();
	static char* to_text(int track_usage);
	static int from_text(char *text);
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
	static char* to_text(int shape);
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

class TimeFrontFrameRange : public BC_ISlider
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


class TimeFrontWindow : public BC_Window
{
public:
	TimeFrontWindow(TimeFrontMain *plugin, int x, int y);
	~TimeFrontWindow();
	
	int create_objects();
	int close_event();
	void update_shape();

	TimeFrontMain *plugin;
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
};



PLUGIN_THREAD_HEADER(TimeFrontMain, TimeFrontThread, TimeFrontWindow)


class TimeFrontMain : public PluginVClient
{
public:
	TimeFrontMain(PluginServer *server);
	~TimeFrontMain();

//	int process_realtime(VFrame *input_ptr, VFrame *output_ptr);
	int process_buffer(VFrame **frame,
		int64_t start_position,
		double frame_rate);

	int is_realtime();
	int is_multichannel();
	int load_defaults();
	int save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void update_gui();
	int is_synthesis();

	PLUGIN_CLASS_MEMBERS(TimeFrontConfig, TimeFrontThread)

	int need_reconfigure;

	OverlayFrame *overlayer;
	VFrame *gradient;
	VFrame *input, *output;
	TimeFrontServer *engine;
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
	YUV yuv;
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
