// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef DESPIKE_H
#define DESPIKE_H

#define PLUGIN_TITLE N_("Despike")
#define PLUGIN_IS_AUDIO
#define PLUGIN_IS_REALTIME
#define PLUGIN_USES_TMPFRAME

#define PLUGIN_CLASS Despike
#define PLUGIN_CONFIG_CLASS DespikeConfig
#define PLUGIN_THREAD_CLASS DespikeThread
#define PLUGIN_GUI_CLASS DespikeWindow

class DespikeEngine;

#include "pluginmacros.h"
#include "despikewindow.h"
#include "language.h"
#include "pluginaclient.h"

class DespikeConfig
{
public:
	DespikeConfig();

	int equivalent(DespikeConfig &that);
	void copy_from(DespikeConfig &that);
	void interpolate(DespikeConfig &prev, 
		DespikeConfig &next, 
		ptstime prev_pts,
		ptstime next_pts,
		ptstime current_pts);

	double level;
	double slope;
	PLUGIN_CONFIG_CLASS_MEMBERS
};

class Despike : public PluginAClient
{
public:
	Despike(PluginServer *server);
	~Despike();

	PLUGIN_CLASS_MEMBERS

	DB db;

	AFrame *process_tmpframe(AFrame *input);
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);

// non realtime support
	void load_defaults();
	void save_defaults();

	double last_sample;
};

#endif
