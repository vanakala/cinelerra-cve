// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>


#ifndef IRISSQUARE_H
#define IRISSQUARE_H

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_TRANSITION
#define PLUGIN_USES_TMPFRAME

#define PLUGIN_TITLE N_("IrisSquare")
#define PLUGIN_CLASS IrisSquareMain
#define PLUGIN_THREAD_CLASS IrisSquareThread
#define PLUGIN_GUI_CLASS IrisSquareWindow

#include "pluginmacros.h"

#include "language.h"
#include "pluginvclient.h"
#include "pluginwindow.h"
#include "vframe.inc"

class IrisSquareIn : public BC_Radial
{
public:
	IrisSquareIn(IrisSquareMain *plugin, 
		IrisSquareWindow *window,
		int x,
		int y);
	int handle_event();
	IrisSquareMain *plugin;
	IrisSquareWindow *window;
};

class IrisSquareOut : public BC_Radial
{
public:
	IrisSquareOut(IrisSquareMain *plugin, 
		IrisSquareWindow *window,
		int x,
		int y);
	int handle_event();
	IrisSquareMain *plugin;
	IrisSquareWindow *window;
};

class IrisSquareWindow : public PluginWindow
{
public:
	IrisSquareWindow(IrisSquareMain *plugin, int x, int y);

	PLUGIN_GUI_CLASS_MEMBERS
	IrisSquareIn *in;
	IrisSquareOut *out;
};

PLUGIN_THREAD_HEADER

class IrisSquareMain : public PluginVClient
{
public:
	IrisSquareMain(PluginServer *server);
	~IrisSquareMain();

	PLUGIN_CLASS_MEMBERS

// required for all realtime plugins
	void process_realtime(VFrame *incoming, VFrame *outgoing);
	void load_defaults();
	void save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	int load_configuration();

	int direction;
};

#endif
