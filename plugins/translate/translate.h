// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef TRANSLATE_H
#define TRANSLATE_H

// the simplest plugin possible
#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME
#define PLUGIN_USES_TMPFRAME

#define PLUGIN_TITLE N_("Translate")
#define PLUGIN_CLASS TranslateMain
#define PLUGIN_CONFIG_CLASS TranslateConfig
#define PLUGIN_THREAD_CLASS TranslateThread
#define PLUGIN_GUI_CLASS TranslateWin

#include "pluginmacros.h"

#include "bchash.h"
#include "language.h"
#include "translatewin.h"
#include "overlayframe.h"
#include "pluginvclient.h"

class TranslateConfig
{
public:
	TranslateConfig();
	int equivalent(TranslateConfig &that);
	void copy_from(TranslateConfig &that);
	void interpolate(TranslateConfig &prev, 
		TranslateConfig &next, 
		ptstime prev_pts,
		ptstime next_pts,
		ptstime current_pts);

	double in_x;
	double in_y;
	double in_w;
	double in_h;
	double out_x;
	double out_y;
	double out_w;
	double out_h;
	PLUGIN_CONFIG_CLASS_MEMBERS
};


class TranslateMain : public PluginVClient
{
public:
	TranslateMain(PluginServer *server);
	~TranslateMain();

	PLUGIN_CLASS_MEMBERS

// required for all realtime plugins
	VFrame *process_tmpframe(VFrame *input);
	void reset_plugin();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void load_defaults();
	void save_defaults();

	OverlayFrame *overlayer;   // To translate images
};

#endif
