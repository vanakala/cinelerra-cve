// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef REFRAMERT_H
#define REFRAMERT_H

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME
#define PLUGIN_USES_TMPFRAME
#define PLUGIN_IS_SYNTHESIS

#define PLUGIN_TITLE N_("ReframeRT")
#define PLUGIN_CLASS ReframeRT
#define PLUGIN_CONFIG_CLASS ReframeRTConfig
#define PLUGIN_THREAD_CLASS ReframeRTThread
#define PLUGIN_GUI_CLASS ReframeRTWindow

#include "pluginmacros.h"

#include "bctextbox.h"
#include "bctoggle.h"
#include "language.h"
#include "pluginvclient.h"
#include "pluginwindow.h"

class ReframeRTConfig
{
public:
	ReframeRTConfig();

	void boundaries();
	int equivalent(ReframeRTConfig &src);
	void copy_from(ReframeRTConfig &src);
	void interpolate(ReframeRTConfig &prev, 
		ReframeRTConfig &next, 
		ptstime prev_pts,
		ptstime next_pts,
		ptstime current_pts);
	double scale;
	int stretch;
	int interp;
	PLUGIN_CONFIG_CLASS_MEMBERS
};


class ReframeRTScale : public BC_TumbleTextBox
{
public:
	ReframeRTScale(ReframeRT *plugin,
		ReframeRTWindow *gui,
		int x,
		int y);
	int handle_event();
	ReframeRT *plugin;
};


class ReframeRTStretch : public BC_Radial
{
public:
	ReframeRTStretch(ReframeRT *plugin,
		ReframeRTWindow *gui,
		int x,
		int y);
	int handle_event();
	ReframeRT *plugin;
	ReframeRTWindow *gui;
};


class ReframeRTDownsample : public BC_Radial
{
public:
	ReframeRTDownsample(ReframeRT *plugin,
		ReframeRTWindow *gui,
		int x,
		int y);
	int handle_event();
	ReframeRT *plugin;
	ReframeRTWindow *gui;
};


class ReframeRTInterpolate : public BC_CheckBox
{
public:
	ReframeRTInterpolate(ReframeRT *plugin,
		ReframeRTWindow *gui,
		int x,
		int y);
	int handle_event();
	ReframeRT *plugin;
	ReframeRTWindow *gui;
};


class ReframeRTWindow : public PluginWindow
{
public:
	ReframeRTWindow(ReframeRT *plugin, int x, int y);

	void update();

	ReframeRTScale *scale;
	ReframeRTStretch *stretch;
	ReframeRTDownsample *downsample;
	ReframeRTInterpolate *interpolate;
	PLUGIN_GUI_CLASS_MEMBERS
};

PLUGIN_THREAD_HEADER

class ReframeRT : public PluginVClient
{
public:
	ReframeRT(PluginServer *server);
	~ReframeRT();

	PLUGIN_CLASS_MEMBERS

	void load_defaults();
	void save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	VFrame *process_tmpframe(VFrame *frame);
};

#endif
