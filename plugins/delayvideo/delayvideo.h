// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef DELAYVIDEO_H
#define DELAYVIDEO_H

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME
#define PLUGIN_USES_TMPFRAME

// Old name was "Delay Video"
#define PLUGIN_TITLE N_("Delay")
#define PLUGIN_CLASS DelayVideo
#define PLUGIN_CONFIG_CLASS DelayVideoConfig
#define PLUGIN_THREAD_CLASS DelayVideoThread
#define PLUGIN_GUI_CLASS DelayVideoWindow

#include "pluginmacros.h"

#include "bchash.inc"
#include "bctextbox.h"
#include "language.h"
#include "pluginvclient.h"
#include "vframe.inc"
#include "pluginwindow.h"


class DelayVideoConfig
{
public:
	DelayVideoConfig();

	int equivalent(DelayVideoConfig &that);
	void copy_from(DelayVideoConfig &that);
	void interpolate(DelayVideoConfig &prev, 
		DelayVideoConfig &next, 
		ptstime prev_pts,
		ptstime next_pts,
		ptstime current_pts);

	ptstime length;
	PLUGIN_CONFIG_CLASS_MEMBERS
};


class DelayVideoSlider : public BC_TextBox
{
public:
	DelayVideoSlider(DelayVideo *plugin, int x, int y);

	int handle_event();

	DelayVideo *plugin;
};


class DelayVideoWindow : public PluginWindow
{
public:
	DelayVideoWindow(DelayVideo *plugin, int x, int y);

	void update();

	DelayVideoSlider *slider;
	PLUGIN_GUI_CLASS_MEMBERS
};


PLUGIN_THREAD_HEADER


class DelayVideo : public PluginVClient
{
public:
	DelayVideo(PluginServer *server);
	~DelayVideo();

	PLUGIN_CLASS_MEMBERS

	VFrame *process_tmpframe(VFrame *input);

	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);

	void load_defaults();
	void save_defaults();
};

#endif
