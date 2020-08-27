// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2020 Einar Rünkaru <einarrunkaru@gmail dot com>

// Draws Waveform
#ifndef WAVEFORM_H
#define WAVEFORM_H

#define PLUGIN_IS_AUDIO
#define PLUGIN_IS_REALTIME
#define PLUGIN_USES_TMPFRAME
#define PLUGIN_IS_MULTICHANNEL
#define PLUGIN_CUSTOM_LOAD_CONFIGURATION
#define PLUGIN_NOT_KEYFRAMEABLE

#define PLUGIN_TITLE N_("Waveform")
#define PLUGIN_CLASS WaveForm
#define PLUGIN_CONFIG_CLASS WaveFormConfig
#define PLUGIN_THREAD_CLASS WaveFormThread
#define PLUGIN_GUI_CLASS WaveFormWindow

#include "pluginmacros.h"

#include "aframe.inc"
#include "bcsubwindow.inc"
#include "bcpot.h"
#include "cinelerra.h"
#include "language.h"
#include "pluginaclient.h"
#include "pluginwindow.h"

class WaveForm;

class WaveFormConfig
{
public:
	WaveFormConfig();

	double scale;
	PLUGIN_CONFIG_CLASS_MEMBERS
};


class WaveFormatScale : public BC_FPot
{
public:
	WaveFormatScale(int x, int y, WaveForm *plugin);

	int handle_event();
private:
	WaveForm *plugin;
};

class WaveFormWindow : public PluginWindow
{
public:
	WaveFormWindow(WaveForm *plugin, int x, int y);

	void update();
	void draw_framewave(AFrame *frame, BC_SubWindow *canvas);
	int draw_timeline(BC_SubWindow *canvas);
	void clear_waveforms();

	int numcanvas;
	int canvas_left;
	int canvas_top;
	int canvas_fullh;
	int timeline_top;
	ptstime timeline_start;
	ptstime timeline_length;
	BC_SubWindow *canvas[MAX_CHANNELS];
	WaveFormatScale *scale;

	PLUGIN_GUI_CLASS_MEMBERS
};


class WaveFormClearButton : public BC_GenericButton
{
public:
	WaveFormClearButton(int x, int y, WaveFormWindow *parent);

	int handle_event();
private:
	WaveFormWindow *parentwindow;
};

PLUGIN_THREAD_HEADER

class WaveForm : public PluginAClient
{
public:
	WaveForm(PluginServer *server);
	~WaveForm();

	PLUGIN_CLASS_MEMBERS

	void process_tmpframes(AFrame **frames);

	void load_defaults();
	void save_defaults();

	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);

	AFrame **aframes;
	int framecount;
};

#endif
