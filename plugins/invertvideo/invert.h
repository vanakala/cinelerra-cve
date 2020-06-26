// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef INVERT_H
#define INVERT_H

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME
#define PLUGIN_USES_TMPFRAME

// Old name was "Invert Video"

#define PLUGIN_TITLE N_("Invert")
#define PLUGIN_CLASS InvertVideoEffect
#define PLUGIN_CONFIG_CLASS InvertVideoConfig
#define PLUGIN_THREAD_CLASS InvertVideoThread
#define PLUGIN_GUI_CLASS InvertVideoWindow

#define GL_GLEXT_PROTOTYPES

#include "pluginmacros.h"

#include "bctoggle.h"
#include "language.h"
#include "pluginvclient.h"
#include "pluginwindow.h"
#include "vframe.inc"

class InvertVideoConfig
{
public:
	InvertVideoConfig();

	void copy_from(InvertVideoConfig &src);
	int equivalent(InvertVideoConfig &src);
	void interpolate(InvertVideoConfig &prev, 
		InvertVideoConfig &next, 
		ptstime prev_pts,
		ptstime next_pts,
		ptstime current_pts);

	int chan0;
	int chan1;
	int chan2;
	int chan3;
	PLUGIN_CONFIG_CLASS_MEMBERS
};

class InvertVideoEnable : public BC_CheckBox
{
public:
	InvertVideoEnable(InvertVideoEffect *plugin, int *output,
		int x, int y, const char *text);

	int handle_event();

	InvertVideoEffect *plugin;
	int *output;
};

class InvertVideoWindow : public PluginWindow
{
public:
	InvertVideoWindow(InvertVideoEffect *plugin, int x, int y);

	void update();

	InvertVideoEnable *chan0;
	InvertVideoEnable *chan1;
	InvertVideoEnable *chan2;
	InvertVideoEnable *chan3;
	PLUGIN_GUI_CLASS_MEMBERS
private:
	static const char *inv_chn_rgba[];
	static const char *inv_chn_ayuv[];
};

PLUGIN_THREAD_HEADER

class InvertVideoEffect : public PluginVClient
{
public:
	InvertVideoEffect(PluginServer *server);
	~InvertVideoEffect();

	PLUGIN_CLASS_MEMBERS

	VFrame *process_tmpframe(VFrame *frame);

	void load_defaults();
	void save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void handle_opengl();
};

#endif
