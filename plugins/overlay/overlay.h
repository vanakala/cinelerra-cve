// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME
#define PLUGIN_IS_MULTICHANNEL
#define PLUGIN_IS_SYNTHESIS
#define PLUGIN_USES_TMPFRAME
#define PLUGIN_CUSTOM_LOAD_CONFIGURATION

#define PLUGIN_TITLE N_("Overlay")
#define PLUGIN_CLASS Overlay
#define PLUGIN_CONFIG_CLASS OverlayConfig
#define PLUGIN_THREAD_CLASS OverlayThread
#define PLUGIN_GUI_CLASS OverlayWindow

#include "pluginmacros.h"

#include "keyframe.inc"
#include "overlayframe.inc"
#include "pluginvclient.h"
#include "pluginwindow.h"
#include "vframe.inc"

#include <string.h>
#include <stdint.h>


class OverlayConfig
{
public:
	OverlayConfig();

	static const char* mode_to_text(int mode);
	int mode;

	PLUGIN_CONFIG_CLASS_MEMBERS
};


class OverlayMode : public BC_PopupMenu
{
public:
	OverlayMode(Overlay *plugin,
		int x, 
		int y);
	void create_objects();
	int handle_event();
	Overlay *plugin;
};


class OverlayWindow : public PluginWindow
{
public:
	OverlayWindow(Overlay *plugin, int x, int y);

	void update();

	OverlayMode *mode;
	PLUGIN_GUI_CLASS_MEMBERS
};


PLUGIN_THREAD_HEADER


class Overlay : public PluginVClient
{
public:
	Overlay(PluginServer *server);
	~Overlay();

	PLUGIN_CLASS_MEMBERS

	void process_tmpframes(VFrame **frame);
	void reset_plugin();
	void load_defaults();
	void save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void handle_opengl();

	OverlayFrame *overlayer;
};
