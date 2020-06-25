// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef FLIP_H
#define FLIP_H

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME
#define PLUGIN_USES_TMPFRAME

#define PLUGIN_TITLE N_("Flip")
#define PLUGIN_CLASS FlipMain
#define PLUGIN_CONFIG_CLASS FlipConfig
#define PLUGIN_THREAD_CLASS FlipThread
#define PLUGIN_GUI_CLASS FlipWindow

#include "pluginmacros.h"
#include "filexml.h"
#include "flipwindow.h"
#include "language.h"
#include "pluginvclient.h"

class FlipConfig
{
public:
	FlipConfig();
	void copy_from(FlipConfig &that);
	int equivalent(FlipConfig &that);
	void interpolate(FlipConfig &prev, 
		FlipConfig &next,
		ptstime prev_pts,
		ptstime next_pts,
		ptstime current_pts);
	int flip_horizontal;
	int flip_vertical;
	PLUGIN_CONFIG_CLASS_MEMBERS
};

class FlipMain : public PluginVClient
{
public:
	FlipMain(PluginServer *server);
	~FlipMain();

	PLUGIN_CLASS_MEMBERS;

	VFrame *process_tmpframe(VFrame *frame);
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void load_defaults();
	void save_defaults();
	void handle_opengl();
};
#endif
