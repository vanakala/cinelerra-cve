// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef FREEVERB_H
#define FREEVERB_H

#define PLUGIN_TITLE N_("Freeverb")
#define PLUGIN_IS_AUDIO
#define PLUGIN_IS_REALTIME
#define PLUGIN_IS_MULTICHANNEL
#define PLUGIN_USES_TMPFRAME

#define PLUGIN_CLASS FreeverbEffect
#define PLUGIN_CONFIG_CLASS FreeverbConfig
#define PLUGIN_THREAD_CLASS FreeverbThread
#define PLUGIN_GUI_CLASS FreeverbWindow

#include "pluginmacros.h"

#include "aframe.inc"
#include "bcpot.h"
#include "bctoggle.h"
#include "language.h"
#include "pluginaclient.h"
#include "pluginwindow.h"
#include "revmodel.hpp"

class FreeverbConfig
{
public:
	FreeverbConfig();

	int equivalent(FreeverbConfig &that);
	void copy_from(FreeverbConfig &that);
	void interpolate(FreeverbConfig &prev, 
		FreeverbConfig &next, 
		ptstime prev_pts,
		ptstime next_pts,
		ptstime current_pts);

	float gain;
	float roomsize;
	float damp;
	float wet;
	float dry;
	float width;
	int mode;
	PLUGIN_CONFIG_CLASS_MEMBERS
};


class FreeverbGain : public BC_FPot
{
public:
	FreeverbGain(FreeverbEffect *plugin, int x, int y);
	int handle_event();
	FreeverbEffect *plugin;
};

class FreeverbRoomsize : public BC_FPot
{
public:
	FreeverbRoomsize(FreeverbEffect *plugin, int x, int y);
	int handle_event();
	FreeverbEffect *plugin;
};

class FreeverbDamp : public BC_FPot
{
public:
	FreeverbDamp(FreeverbEffect *plugin, int x, int y);
	int handle_event();
	FreeverbEffect *plugin;
};

class FreeverbWet : public BC_FPot
{
public:
	FreeverbWet(FreeverbEffect *plugin, int x, int y);
	int handle_event();
	FreeverbEffect *plugin;
};

class FreeverbDry : public BC_FPot
{
public:
	FreeverbDry(FreeverbEffect *plugin, int x, int y);
	int handle_event();
	FreeverbEffect *plugin;
};

class FreeverbWidth : public BC_FPot
{
public:
	FreeverbWidth(FreeverbEffect *plugin, int x, int y);
	int handle_event();
	FreeverbEffect *plugin;
};

class FreeverbMode : public BC_CheckBox
{
public:
	FreeverbMode(FreeverbEffect *plugin, int x, int y);
	int handle_event();
	FreeverbEffect *plugin;
};


class FreeverbWindow : public PluginWindow
{
public:
	FreeverbWindow(FreeverbEffect *plugin, int x, int y);

	void update();

	FreeverbGain *gain;
	FreeverbRoomsize *roomsize;
	FreeverbDamp *damp;
	FreeverbWet *wet;
	FreeverbDry *dry;
	FreeverbWidth *width;
	FreeverbMode *mode;
	PLUGIN_GUI_CLASS_MEMBERS
};

PLUGIN_THREAD_HEADER

class FreeverbEffect : public PluginAClient
{
public:
	FreeverbEffect(PluginServer *server);
	~FreeverbEffect();

	PLUGIN_CLASS_MEMBERS

	void reset_plugin();
	void read_data(KeyFrame *keyframe);
	void save_data(KeyFrame *keyframe);
	void process_tmpframes(AFrame **input);

	void load_defaults();
	void save_defaults();

	revmodel *engine;
	float **temp;
	float **temp_out;
	int temp_allocated;
};

#endif
