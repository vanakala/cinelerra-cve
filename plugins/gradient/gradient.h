// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef GRADIENT_H
#define GRADIENT_H

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME
#define PLUGIN_IS_SYNTHESIS
#define PLUGIN_USES_TMPFRAME

#define PLUGIN_TITLE N_("Gradient")
#define PLUGIN_CLASS GradientMain
#define PLUGIN_CONFIG_CLASS GradientConfig
#define PLUGIN_THREAD_CLASS GradientThread
#define PLUGIN_GUI_CLASS GradientWindow

#include "pluginmacros.h"

class GradientEngine;
class GradientThread;
class GradientWindow;
class GradientServer;

#include "bcpopupmenu.h"
#include "bcpot.h"
#include "colorpicker.h"
#include "bchash.inc"
#include "filexml.inc"
#include "language.h"
#include "loadbalance.h"
#include "overlayframe.inc"
#include "pluginvclient.h"
#include "pluginwindow.h"
#include "thread.h"
#include "vframe.inc"

class GradientConfig
{
public:
	GradientConfig();

	int equivalent(GradientConfig &that);
	void copy_from(GradientConfig &that);
	void interpolate(GradientConfig &prev, 
		GradientConfig &next, 
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
		SQUARE
	};

	double center_x;
	double center_y;
	double angle;
	double in_radius;
	double out_radius;
	int in_r, in_g, in_b, in_a;
	int out_r, out_g, out_b, out_a;
	PLUGIN_CONFIG_CLASS_MEMBERS
};


class GradientShape : public BC_PopupMenu
{
public:
	GradientShape(GradientMain *plugin, 
		GradientWindow *gui,
		int x, 
		int y);

	static char* to_text(int shape);
	static int from_text(char *text);
	int handle_event();
	GradientMain *plugin;
	GradientWindow *gui;
};


class GradientRate : public BC_PopupMenu
{
public:
	GradientRate(GradientMain *plugin, 
		int x, 
		int y);

	static const char* to_text(int shape);
	static int from_text(const char *text);
	int handle_event();
	GradientMain *plugin;
	GradientWindow *gui;
};

class GradientCenterX : public BC_FPot
{
public:
	GradientCenterX(GradientMain *plugin, int x, int y);
	int handle_event();
	GradientMain *plugin;
};

class GradientCenterY : public BC_FPot
{
public:
	GradientCenterY(GradientMain *plugin, int x, int y);
	int handle_event();
	GradientMain *plugin;
};

class GradientAngle : public BC_FPot
{
public:
	GradientAngle(GradientMain *plugin, int x, int y);
	int handle_event();
	GradientMain *plugin;
};

class GradientInRadius : public BC_FSlider
{
public:
	GradientInRadius(GradientMain *plugin, int x, int y);
	int handle_event();
	GradientMain *plugin;
};

class GradientOutRadius : public BC_FSlider
{
public:
	GradientOutRadius(GradientMain *plugin, int x, int y);
	int handle_event();
	GradientMain *plugin;
};

class GradientInColorButton : public BC_GenericButton
{
public:
	GradientInColorButton(GradientMain *plugin, GradientWindow *window, int x, int y);
	int handle_event();
	GradientMain *plugin;
	GradientWindow *window;
};


class GradientOutColorButton : public BC_GenericButton
{
public:
	GradientOutColorButton(GradientMain *plugin, GradientWindow *window, int x, int y);
	int handle_event();
	GradientMain *plugin;
	GradientWindow *window;
};


class GradientInColorThread : public ColorThread
{
public:
	GradientInColorThread(GradientMain *plugin, GradientWindow *window);
	virtual int handle_new_color(int output, int alpha);
	GradientMain *plugin;
	GradientWindow *window;
};


class GradientOutColorThread : public ColorThread
{
public:
	GradientOutColorThread(GradientMain *plugin, GradientWindow *window);
	virtual int handle_new_color(int output, int alpha);
	GradientMain *plugin;
	GradientWindow *window;
};


class GradientWindow : public PluginWindow
{
public:
	GradientWindow(GradientMain *plugin, int x, int y);
	~GradientWindow();

	void update();
	void update_in_color();
	void update_out_color();
	void update_shape();

	BC_Title *angle_title;
	GradientAngle *angle;
	GradientInRadius *in_radius;
	GradientOutRadius *out_radius;
	GradientInColorButton *in_color;
	GradientOutColorButton *out_color;
	GradientInColorThread *in_color_thread;
	GradientOutColorThread *out_color_thread;
	GradientShape *shape;
	BC_Title *shape_title;
	GradientCenterX *center_x;
	BC_Title *center_x_title;
	BC_Title *center_y_title;
	GradientCenterY *center_y;
	GradientRate *rate;
	int in_color_x, in_color_y;
	int out_color_x, out_color_y;
	int shape_x, shape_y;
	PLUGIN_GUI_CLASS_MEMBERS
};


PLUGIN_THREAD_HEADER


class GradientMain : public PluginVClient
{
public:
	GradientMain(PluginServer *server);
	~GradientMain();

	VFrame *process_tmpframe(VFrame *frame);
	void reset_plugin();
	void load_defaults();
	void save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void handle_opengl();

	PLUGIN_CLASS_MEMBERS

	OverlayFrame *overlayer;
	VFrame *gradient;
	VFrame *output;
	GradientServer *engine;
};

class GradientPackage : public LoadPackage
{
public:
	GradientPackage();
	int y1;
	int y2;
};

class GradientUnit : public LoadClient
{
public:
	GradientUnit(GradientServer *server, GradientMain *plugin);
	~GradientUnit();

	void process_package(LoadPackage *package);
	GradientServer *server;
	GradientMain *plugin;

	int gradient_size;
	uint16_t *r_table;
	uint16_t *g_table;
	uint16_t *b_table;
	uint16_t *a_table;
};

class GradientServer : public LoadServer
{
public:
	GradientServer(GradientMain *plugin, int total_clients, int total_packages);

	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
	GradientMain *plugin;
};
#endif
