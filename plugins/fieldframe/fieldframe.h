// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef FIELDFRAME_H
#define FIELDFRAME_H

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME
#define PLUGIN_USES_TMPFRAME
#define PLUGIN_CUSTOM_LOAD_CONFIGURATION

#define PLUGIN_TITLE N_("Fields to frames")
#define PLUGIN_CLASS FieldFrame
#define PLUGIN_CONFIG_CLASS FieldFrameConfig
#define PLUGIN_THREAD_CLASS FieldFrameThread
#define PLUGIN_GUI_CLASS FieldFrameWindow

#include "pluginmacros.h"

#include "bctoggle.h"
#include "keyframe.inc"
#include "language.h"
#include "pluginvclient.h"
#include "pluginwindow.h"
#include "vframe.inc"

#define TOP_FIELD_FIRST 0
#define BOTTOM_FIELD_FIRST 1

class FieldFrameConfig
{
public:
	FieldFrameConfig();

	int equivalent(FieldFrameConfig &src);
	int field_dominance;
	PLUGIN_CONFIG_CLASS_MEMBERS
};

class FieldFrameTop : public BC_Radial
{
public:
	FieldFrameTop(FieldFrame *plugin, FieldFrameWindow *gui, int x, int y);

	int handle_event();
	FieldFrame *plugin;
	FieldFrameWindow *gui;
};


class FieldFrameBottom : public BC_Radial
{
public:
	FieldFrameBottom(FieldFrame *plugin, FieldFrameWindow *gui, int x, int y);

	int handle_event();
	FieldFrame *plugin;
	FieldFrameWindow *gui;
};


class FieldFrameWindow : public PluginWindow
{
public:
	FieldFrameWindow(FieldFrame *plugin, int x, int y);

	void update();

	FieldFrameTop *top;
	FieldFrameBottom *bottom;
	PLUGIN_GUI_CLASS_MEMBERS
};


PLUGIN_THREAD_HEADER


class FieldFrame : public PluginVClient
{
public:
	FieldFrame(PluginServer *server);
	~FieldFrame();

	PLUGIN_CLASS_MEMBERS

	VFrame *process_tmpframe(VFrame *frame);

	void load_defaults();
	void save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void apply_field(VFrame *output, VFrame *input, int field);

	VFrame *input;
};

#endif
