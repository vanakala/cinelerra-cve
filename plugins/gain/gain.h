// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef GAIN_H
#define GAIN_H

#define PLUGIN_IS_AUDIO
#define PLUGIN_IS_REALTIME
#define PLUGIN_USES_TMPFRAME

#define PLUGIN_TITLE N_("Gain")
#define PLUGIN_CLASS Gain
#define PLUGIN_CONFIG_CLASS GainConfig
#define PLUGIN_THREAD_CLASS GainThread
#define PLUGIN_GUI_CLASS GainWindow

#include "language.h"
#include "pluginmacros.h"
#include "gainwindow.h"
#include "pluginaclient.h"

class GainConfig
{
public:
	GainConfig();
	int equivalent(GainConfig &that);
	void copy_from(GainConfig &that);
	void interpolate(GainConfig &prev, 
		GainConfig &next, 
		ptstime prev_pts,
		ptstime next_pts,
		ptstime current_pts);

	double level;
	double levelslope;
	PLUGIN_CONFIG_CLASS_MEMBERS
};

class Gain : public PluginAClient
{
public:
	Gain(PluginServer *server);
	~Gain();

	AFrame *process_tmpframe(AFrame *input);

	PLUGIN_CLASS_MEMBERS
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void load_defaults();
	void save_defaults();

	DB db;
};

#endif
