// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME
#define PLUGIN_USES_TMPFRAME

#define PLUGIN_TITLE N_("Perspective")
#define PLUGIN_CLASS PerspectiveMain
#define PLUGIN_CONFIG_CLASS PerspectiveConfig
#define PLUGIN_THREAD_CLASS PerspectiveThread
#define PLUGIN_GUI_CLASS PerspectiveWindow

#include "pluginmacros.h"

#include "keyframe.inc"
#include "picon_png.h"
#include "pluginvclient.h"
#include "pluginwindow.h"
#include "vframe.inc"


class PerspectiveConfig
{
public:
	PerspectiveConfig();

	int equivalent(PerspectiveConfig &that);
	void copy_from(PerspectiveConfig &that);
	void interpolate(PerspectiveConfig &prev, 
		PerspectiveConfig &next, 
		ptstime prev_pts,
		ptstime next_pts,
		ptstime current_pts);

	double x1, y1;
	double x2, y2;
	double x3, y3;
	double x4, y4;
	int mode;
	int current_point;
	int forward;
	PLUGIN_CONFIG_CLASS_MEMBERS
};

class PerspectiveCanvas : public BC_SubWindow
{
public:
	PerspectiveCanvas(PerspectiveMain *plugin, 
		int x, 
		int y, 
		int w,
		int h);
	int button_press_event();
	int button_release_event();
	int cursor_motion_event();
	int state;

	enum
	{
		NONE,
		DRAG,
		DRAG_FULL,
		ZOOM
	};

	int start_cursor_x, start_cursor_y;
	double start_x1, start_y1;
	double start_x2, start_y2;
	double start_x3, start_y3;
	double start_x4, start_y4;
	PerspectiveMain *plugin;
};

class PerspectiveCoord : public BC_TumbleTextBox
{
public:
	PerspectiveCoord(PerspectiveWindow *gui,
		PerspectiveMain *plugin, 
		int x, 
		int y,
		double value,
		int is_x);

	int handle_event();
	PerspectiveMain *plugin;
	int is_x;
};

class PerspectiveReset : public BC_GenericButton
{
public:
	PerspectiveReset(PerspectiveMain *plugin, 
		int x, 
		int y);

	int handle_event();
	PerspectiveMain *plugin;
};

class PerspectiveMode : public BC_Radial
{
public:
	PerspectiveMode(PerspectiveMain *plugin, 
		int x, 
		int y,
		int value,
		char *text);

	int handle_event();
	PerspectiveMain *plugin;
	int value;
};

class PerspectiveDirection : public BC_Radial
{
public:
	PerspectiveDirection(PerspectiveMain *plugin, 
		int x, 
		int y,
		int value,
		char *text);

	int handle_event();
	PerspectiveMain *plugin;
	int value;
};

class PerspectiveWindow : public PluginWindow
{
public:
	PerspectiveWindow(PerspectiveMain *plugin, int x, int y);

	void update();
	void update_canvas();
	void update_mode();
	void update_coord();
	void calculate_canvas_coords(int &x1, 
		int &y1, 
		int &x2, 
		int &y2, 
		int &x3, 
		int &y3, 
		int &x4, 
		int &y4);

	PerspectiveCanvas *canvas;
	PerspectiveCoord *x, *y;
	PerspectiveReset *reset;
	PerspectiveMode *mode_perspective, *mode_sheer, *mode_stretch;
	PerspectiveDirection *forward, *reverse;
	PLUGIN_GUI_CLASS_MEMBERS
};

PLUGIN_THREAD_HEADER

class PerspectiveMain : public PluginVClient
{
public:
	PerspectiveMain(PluginServer *server);
	~PerspectiveMain();

	VFrame *process_tmpframe(VFrame *frame);
	void reset_plugin();
	void load_defaults();
	void save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void handle_opengl();

	PLUGIN_CLASS_MEMBERS

	double get_current_x();
	double get_current_y();
	void set_current_x(double value);
	void set_current_y(double value);

	VFrame *input, *output;
	VFrame *strech_temp;
	AffineEngine *engine;
};
