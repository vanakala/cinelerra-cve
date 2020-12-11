// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef SOUNDLEVEL_H
#define SOUNDLEVEL_H

#define PLUGIN_TITLE N_("SoundLevel")
#define PLUGIN_IS_AUDIO
#define PLUGIN_IS_REALTIME
#define PLUGIN_USES_TMPFRAME

#define PLUGIN_CLASS SoundLevelEffect
#define PLUGIN_CONFIG_CLASS SoundLevelConfig
#define PLUGIN_THREAD_CLASS SoundLevelThread
#define PLUGIN_GUI_CLASS SoundLevelWindow

#include "pluginmacros.h"
#include "bcslider.h"
#include "language.h"
#include "pluginaclient.h"
#include "pluginwindow.h"

class SoundLevelConfig
{
public:
	SoundLevelConfig();
	void copy_from(SoundLevelConfig &that);
	int equivalent(SoundLevelConfig &that);
	void interpolate(SoundLevelConfig &prev, 
		SoundLevelConfig &next, 
		ptstime prev_pts,
		ptstime next_pts,
		ptstime current_pts);
	double duration;
	PLUGIN_CONFIG_CLASS_MEMBERS
};

class SoundLevelDuration : public BC_FSlider
{
public:
	SoundLevelDuration(SoundLevelEffect *plugin, int x, int y);
	int handle_event();
	SoundLevelEffect *plugin;
};

class SoundLevelWindow : public PluginWindow
{
public:
	SoundLevelWindow(SoundLevelEffect *plugin, int x, int y);
	void update();

	BC_Title *soundlevel_max;
	BC_Title *soundlevel_rms;
	SoundLevelDuration *duration;
	PLUGIN_GUI_CLASS_MEMBERS
};

PLUGIN_THREAD_HEADER

class SoundLevelEffect : public PluginAClient
{
public:
	SoundLevelEffect(PluginServer *server);
	~SoundLevelEffect();

	AFrame *process_tmpframe(AFrame *input);

	void read_data(KeyFrame *keyframe);
	void save_data(KeyFrame *keyframe);

	void load_defaults();
	void save_defaults();
	void render_gui();

	PLUGIN_CLASS_MEMBERS

	double rms_accum;
	double max_accum;
	int accum_size;
};

#endif
