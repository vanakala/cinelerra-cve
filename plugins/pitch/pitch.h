// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef PITCH_H
#define PITCH_H

#define PLUGIN_TITLE N_("Pitch shift")
#define PLUGIN_IS_AUDIO
#define PLUGIN_IS_REALTIME
#define PLUGIN_USES_TMPFRAME
#define PLUGIN_CLASS PitchEffect
#define PLUGIN_CONFIG_CLASS PitchConfig
#define PLUGIN_THREAD_CLASS PitchThread
#define PLUGIN_GUI_CLASS PitchWindow

#include "pluginmacros.h"

#include "bcpot.h"
#include "fourier.h"
#include "pluginaclient.h"
#include "pluginwindow.h"


class PitchScale : public BC_FPot
{
public:
	PitchScale(PitchEffect *plugin, int x, int y);
	int handle_event();
	PitchEffect *plugin;
};

class PitchWindow : public PluginWindow
{
public:
	PitchWindow(PitchEffect *plugin, int x, int y);

	void update();
	PitchScale *scale;
	PLUGIN_GUI_CLASS_MEMBERS
};


PLUGIN_THREAD_HEADER


class PitchConfig
{
public:
	PitchConfig();

	int equivalent(PitchConfig &that);
	void copy_from(PitchConfig &that);
	void interpolate(PitchConfig &prev, 
		PitchConfig &next, 
		ptstime prev_pts,
		ptstime next_pts,
		ptstime current_pts);

	double scale;
	PLUGIN_CONFIG_CLASS_MEMBERS
};

class PitchEffect : public PluginAClient
{
public:
	PitchEffect(PluginServer *server);
	~PitchEffect();

	PLUGIN_CLASS_MEMBERS

	void reset_plugin();
	void read_data(KeyFrame *keyframe);
	void save_data(KeyFrame *keyframe);

	AFrame *process_tmpframe(AFrame *aframe);

	void load_defaults();
	void save_defaults();

	Pitch *fft;
};

#endif
