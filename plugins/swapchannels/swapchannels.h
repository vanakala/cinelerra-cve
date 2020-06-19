// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef SWAPCHANNELS_H
#define SWAPCHANNELS_H

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME
#define PLUGIN_USES_TMPFRAME
#define PLUGIN_CUSTOM_LOAD_CONFIGURATION

#define PLUGIN_TITLE N_("Swap channels")
#define PLUGIN_CLASS SwapMain
#define PLUGIN_CONFIG_CLASS SwapConfig
#define PLUGIN_THREAD_CLASS SwapThread
#define PLUGIN_GUI_CLASS SwapWindow

#include "pluginmacros.h"

#include "bchash.inc"
#include "bcpopupmenu.h"
#include "bcmenuitem.h"
#include "language.h"
#include "pluginvclient.h"
#include "pluginwindow.h"
#include "selection.h"
#include "vframe.inc"

class SwapConfig
{
public:
	SwapConfig();

	int equivalent(SwapConfig &that);
	void copy_from(SwapConfig &that);

	int chan0;
	int chan1;
	int chan2;
	int chan3;
	PLUGIN_CONFIG_CLASS_MEMBERS
};

class SwapColorSelection : public Selection
{
public:
	SwapColorSelection(int x, int y, struct selection_int *chnls,
		SwapMain *plugin, BC_WindowBase *basewindow, int *value);

	int handle_event();
	void update(int value);
	const char *name(int value);

	static struct selection_int color_channels_rgb[];
	static struct selection_int color_channels_yuv[];
private:
	SwapMain *plugin;
	struct selection_int *channels;
};

class SwapWindow : public PluginWindow
{
public:
	SwapWindow(SwapMain *plugin, int x, int y);

	void update();
private:
	SwapColorSelection *chan0;
	SwapColorSelection *chan1;
	SwapColorSelection *chan2;
	SwapColorSelection *chan3;
	PLUGIN_GUI_CLASS_MEMBERS
};


PLUGIN_THREAD_HEADER

class SwapMain : public PluginVClient
{
public:
	SwapMain(PluginServer *server);
	~SwapMain();

	PLUGIN_CLASS_MEMBERS

	VFrame *process_tmpframe(VFrame *input);

	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);

	void load_defaults();
	void save_defaults();
};

#endif
