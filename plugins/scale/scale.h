// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef SCALE_H
#define SCALE_H

// the simplest plugin possible

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME
#define PLUGIN_USES_TMPFRAME

#define PLUGIN_TITLE N_("Scale")
#define PLUGIN_CLASS ScaleMain
#define PLUGIN_CONFIG_CLASS ScaleConfig
#define PLUGIN_THREAD_CLASS ScaleThread
#define PLUGIN_GUI_CLASS ScaleWin

#include "pluginmacros.h"

class ScaleWidth;
class ScaleHeight;
class ScaleConstrain;

#include "bchash.h"
#include "bctextbox.h"
#include "bctoggle.h"
#include "language.h"
#include "mutex.h"
#include "overlayframe.h"
#include "pluginvclient.h"
#include "pluginwindow.h"

class ScaleConfig
{
public:
	ScaleConfig();

	void copy_from(ScaleConfig &src);
	int equivalent(ScaleConfig &src);
	void interpolate(ScaleConfig &prev, 
		ScaleConfig &next, 
		ptstime prev_pts,
		ptstime next_pts,
		ptstime current_pts);

	double w, h;
	int constrain;
	PLUGIN_CONFIG_CLASS_MEMBERS
};


class ScaleWidth : public BC_TumbleTextBox
{
public:
	ScaleWidth(ScaleWin *win, ScaleMain *client, int x, int y);

	int handle_event();

	ScaleMain *client;
	ScaleWin *win;
};

class ScaleHeight : public BC_TumbleTextBox
{
public:
	ScaleHeight(ScaleWin *win, ScaleMain *client, int x, int y);

	int handle_event();

	ScaleMain *client;
	ScaleWin *win;
};

class ScaleConstrain : public BC_CheckBox
{
public:
	ScaleConstrain(ScaleMain *client, int x, int y);

	int handle_event();

	ScaleMain *client;
};

class ScaleWin : public PluginWindow
{
public:
	ScaleWin(ScaleMain *plugin, int x, int y);

	void update();

	ScaleWidth *width;
	ScaleHeight *height;
	ScaleConstrain *constrain;
	PLUGIN_GUI_CLASS_MEMBERS
};

PLUGIN_THREAD_HEADER

class ScaleMain : public PluginVClient
{
public:
	ScaleMain(PluginServer *server);
	~ScaleMain();

	PLUGIN_CLASS_MEMBERS

	VFrame *process_tmpframe(VFrame *frame);
	void reset_plugin();
	void calculate_transfer(VFrame *frame,
		double &in_x1, double &in_x2,
		double &in_y1, double &in_y2,
		double &out_x1, double &out_x2,
		double &out_y1, double &out_y2);
	void handle_opengl();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void load_defaults();
	void save_defaults();

	OverlayFrame *overlayer;   // To scale images
};
#endif
