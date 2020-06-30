// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef DENOISEVIDEO_H
#define DENOISEVIDEO_H

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME
#define PLUGIN_USES_TMPFRAME

// Old name was "Denoise video"
#define PLUGIN_TITLE N_("Denoise")
#define PLUGIN_CLASS DenoiseVideo
#define PLUGIN_CONFIG_CLASS DenoiseVideoConfig
#define PLUGIN_THREAD_CLASS DenoiseVideoThread
#define PLUGIN_GUI_CLASS DenoiseVideoWindow

#include "pluginmacros.h"

#include "bcslider.h"
#include "bchash.inc"
#include "language.h"
#include "pluginvclient.h"
#include "pluginwindow.h"
#include "vframe.inc"


class DenoiseVideoConfig
{
public:
	DenoiseVideoConfig();

	int equivalent(DenoiseVideoConfig &that);
	void copy_from(DenoiseVideoConfig &that);
	void interpolate(DenoiseVideoConfig &prev, 
		DenoiseVideoConfig &next, 
		ptstime prev_pts,
		ptstime next_pts,
		ptstime current_pts);

	int frames;
	double threshold;
	int chan0;
	int chan1;
	int chan2;
	int chan3;
	PLUGIN_CONFIG_CLASS_MEMBERS
};


class DenoiseVideoFrames : public BC_ISlider
{
public:
	DenoiseVideoFrames(DenoiseVideo *plugin, int x, int y);

	int handle_event();

	DenoiseVideo *plugin;
};


class DenoiseVideoThreshold : public BC_TextBox
{
public:
	DenoiseVideoThreshold(DenoiseVideo *plugin, int x, int y);

	int handle_event();

	DenoiseVideo *plugin;
};


class DenoiseVideoToggle : public BC_CheckBox
{
public:
	DenoiseVideoToggle(DenoiseVideo *plugin, 
		int x, 
		int y, 
		int *output,
		const char *text);

	int handle_event();

	DenoiseVideo *plugin;
	int *output;
};


class DenoiseVideoWindow : public PluginWindow
{
public:
	DenoiseVideoWindow(DenoiseVideo *plugin, int x, int y);

	void update();

	DenoiseVideoFrames *frames;
	DenoiseVideoThreshold *threshold;
	DenoiseVideoToggle *chan0;
	DenoiseVideoToggle *chan1;
	DenoiseVideoToggle *chan2;
	DenoiseVideoToggle *chan3;
	PLUGIN_GUI_CLASS_MEMBERS
private:
	static const char *denoise_chn_rgba[];
	static const char *denoise_chn_ayuv[];
};


PLUGIN_THREAD_HEADER

class DenoiseVideo : public PluginVClient
{
public:
	DenoiseVideo(PluginServer *server);
	~DenoiseVideo();

	PLUGIN_CLASS_MEMBERS

	VFrame *process_tmpframe(VFrame *input);
	void reset_plugin();
	void load_defaults();
	void save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);

	int *accumulation;
};
#endif
