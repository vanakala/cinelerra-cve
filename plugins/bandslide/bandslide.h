// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef BANDSLIDE_H
#define BANDSLIDE_H

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_TRANSITION
#define PLUGIN_USES_TMPFRAME

#define PLUGIN_TITLE N_("BandSlide")
#define PLUGIN_CLASS BandSlideMain
#define PLUGIN_THREAD_CLASS BandSlideThread
#define PLUGIN_GUI_CLASS BandSlideWindow

#include "pluginmacros.h"
#include "language.h"
#include "pluginvclient.h"
#include "pluginwindow.h"
#include "vframe.inc"

class BandSlideCount : public BC_TumbleTextBox
{
public:
	BandSlideCount(BandSlideMain *plugin, 
		BandSlideWindow *window,
		int x,
		int y);
	int handle_event();

	BandSlideMain *plugin;
	BandSlideWindow *window;
};

class BandSlideIn : public BC_Radial
{
public:
	BandSlideIn(BandSlideMain *plugin, 
		BandSlideWindow *window,
		int x,
		int y);
	int handle_event();

	BandSlideMain *plugin;
	BandSlideWindow *window;
};

class BandSlideOut : public BC_Radial
{
public:
	BandSlideOut(BandSlideMain *plugin, 
		BandSlideWindow *window,
		int x,
		int y);
	int handle_event();

	BandSlideMain *plugin;
	BandSlideWindow *window;
};


class BandSlideWindow : public PluginWindow
{
public:
	BandSlideWindow(BandSlideMain *plugin, int x, int y);

	BandSlideCount *count;
	BandSlideIn *in;
	BandSlideOut *out;
	PLUGIN_GUI_CLASS_MEMBERS
};

PLUGIN_THREAD_HEADER

class BandSlideMain : public PluginVClient
{
public:
	BandSlideMain(PluginServer *server);
	~BandSlideMain();

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
