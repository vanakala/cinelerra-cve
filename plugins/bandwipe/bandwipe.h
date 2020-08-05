// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef BANDWIPE_H
#define BANDWIPE_H

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_TRANSITION
#define PLUGIN_USES_TMPFRAME

#define PLUGIN_TITLE N_("BandWipe")
#define PLUGIN_CLASS BandWipeMain
#define PLUGIN_THREAD_CLASS BandWipeThread
#define PLUGIN_GUI_CLASS BandWipeWindow

#include "pluginmacros.h"

#include "language.h"
#include "pluginvclient.h"
#include "pluginwindow.h"
#include "vframe.inc"


class BandWipeCount : public BC_TumbleTextBox
{
public:
	BandWipeCount(BandWipeMain *plugin, 
		BandWipeWindow *window,
		int x,
		int y);
	int handle_event();

	BandWipeMain *plugin;
	BandWipeWindow *window;
};

class BandWipeIn : public BC_Radial
{
public:
	BandWipeIn(BandWipeMain *plugin,
		BandWipeWindow *window,
		int x,
		int y);
	int handle_event();

	BandWipeMain *plugin;
	BandWipeWindow *window;
};

class BandWipeOut : public BC_Radial
{
public:
	BandWipeOut(BandWipeMain *plugin,
		BandWipeWindow *window,
		int x,
		int y);
	int handle_event();

	BandWipeMain *plugin;
	BandWipeWindow *window;
};


class BandWipeWindow : public PluginWindow
{
public:
	BandWipeWindow(BandWipeMain *plugin, int x, int y);

	BandWipeCount *count;
	BandWipeIn *in;
	BandWipeOut *out;
	PLUGIN_GUI_CLASS_MEMBERS
};


PLUGIN_THREAD_HEADER


class BandWipeMain : public PluginVClient
{
public:
	BandWipeMain(PluginServer *server);
	~BandWipeMain();

	PLUGIN_CLASS_MEMBERS
	void process_realtime(VFrame *incoming, VFrame *outgoing);

	int load_configuration();
	void load_defaults();
	void save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);

	int bands;
	int direction;
};

#endif
