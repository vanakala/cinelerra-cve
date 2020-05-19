// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>


#ifndef WIPE_H
#define WIPE_H

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_TRANSITION
#define PLUGIN_USES_TMPFRAME

#define PLUGIN_TITLE N_("Wipe")
#define PLUGIN_CLASS WipeMain
#define PLUGIN_THREAD_CLASS WipeThread
#define PLUGIN_GUI_CLASS WipeWindow

#include "pluginmacros.h"

#include "language.h"
#include "pluginvclient.h"
#include "pluginwindow.h"
#include "vframe.inc"

class WipeLeft : public BC_Radial
{
public:
	WipeLeft(WipeMain *plugin, 
		WipeWindow *window,
		int x,
		int y);
	int handle_event();
	WipeMain *plugin;
	WipeWindow *window;
};

class WipeRight : public BC_Radial
{
public:
	WipeRight(WipeMain *plugin, 
		WipeWindow *window,
		int x,
		int y);
	int handle_event();
	WipeMain *plugin;
	WipeWindow *window;
};


class WipeWindow : public PluginWindow
{
public:
	WipeWindow(WipeMain *plugin, int x, int y);

	WipeLeft *left;
	WipeRight *right;
	PLUGIN_GUI_CLASS_MEMBERS
};

PLUGIN_THREAD_HEADER

class WipeMain : public PluginVClient
{
public:
	WipeMain(PluginServer *server);
	~WipeMain();

	PLUGIN_CLASS_MEMBERS

	void process_realtime(VFrame *incoming, VFrame *outgoing);
	void load_defaults();
	void save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	int load_configuration();

	int direction;
};

#endif
