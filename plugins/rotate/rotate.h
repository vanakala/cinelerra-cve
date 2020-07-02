// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef ROTATE_H
#define ROTATE_H

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME
#define PLUGIN_USES_TMPFRAME

#define PLUGIN_TITLE N_("Rotate")
#define PLUGIN_CLASS RotateEffect
#define PLUGIN_CONFIG_CLASS RotateConfig
#define PLUGIN_THREAD_CLASS RotateThread
#define PLUGIN_GUI_CLASS RotateWindow

#include "pluginmacros.h"

#include "affine.inc"
#include "bcpot.h"
#include "bctextbox.h"
#include "language.h"
#include "picon_png.h"
#include "pluginvclient.h"
#include "pluginwindow.h"
#include "vframe.inc"

class RotateConfig
{
public:
	RotateConfig();

	int equivalent(RotateConfig &that);
	void copy_from(RotateConfig &that);
	void interpolate(RotateConfig &prev, 
		RotateConfig &next, 
		ptstime prev_pts,
		ptstime next_pts,
		ptstime current_pts);

	double angle;
	double pivot_x;
	double pivot_y;
	int draw_pivot;
	PLUGIN_CONFIG_CLASS_MEMBERS
};

class RotateToggle : public BC_Radial
{
public:
	RotateToggle(RotateWindow *window, 
		RotateEffect *plugin, 
		int init_value, 
		int x, 
		int y, 
		int value, 
		const char *string);
	int handle_event();

	RotateEffect *plugin;
	RotateWindow *window;
	int value;
};

class RotateDrawPivot : public BC_CheckBox
{
public:
	RotateDrawPivot(RotateWindow *window, 
		RotateEffect *plugin, 
		int x, 
		int y);
	int handle_event();
	RotateEffect *plugin;
	RotateWindow *window;
	int value;
};


class RotateFine : public BC_FPot
{
public:
	RotateFine(RotateWindow *window, 
		RotateEffect *plugin, 
		int x, 
		int y);
	int handle_event();

	RotateEffect *plugin;
	RotateWindow *window;
};

class RotateX : public BC_FPot
{
public:
	RotateX(RotateWindow *window, 
		RotateEffect *plugin, 
		int x, 
		int y);
	int handle_event();
	RotateEffect *plugin;
	RotateWindow *window;
};

class RotateY : public BC_FPot
{
public:
	RotateY(RotateWindow *window, 
		RotateEffect *plugin, 
		int x, 
		int y);
	int handle_event();
	RotateEffect *plugin;
	RotateWindow *window;
};


class RotateText : public BC_TextBox
{
public:
	RotateText(RotateWindow *window, 
		RotateEffect *plugin, 
		int x, 
		int y);
	int handle_event();

	RotateEffect *plugin;
	RotateWindow *window;
};

class RotateWindow : public PluginWindow
{
public:
	RotateWindow(RotateEffect *plugin, int x, int y);

	void update();
	void update_fine();
	void update_text();
	void update_toggles();

	RotateToggle *toggle0;
	RotateToggle *toggle90;
	RotateToggle *toggle180;
	RotateToggle *toggle270;
	RotateDrawPivot *draw_pivot;
	RotateFine *fine;
	RotateText *text;
	RotateX *x;
	RotateY *y;
	PLUGIN_GUI_CLASS_MEMBERS
};


PLUGIN_THREAD_HEADER


class RotateEffect : public PluginVClient
{
public:
	RotateEffect(PluginServer *server);
	~RotateEffect();

	PLUGIN_CLASS_MEMBERS

	VFrame *process_tmpframe(VFrame *frame);
	void reset_plugin();
	void load_defaults();
	void save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void handle_opengl();

	AffineEngine *engine;
	int need_reconfigure;
};

#endif

