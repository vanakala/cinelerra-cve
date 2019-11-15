
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME
#define PLUGIN_IS_MULTICHANNEL
#define PLUGIN_IS_SYNTHESIS
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

	void process_frame(VFrame **frame);
	void load_defaults();
	void save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void handle_opengl();

	OverlayFrame *overlayer;
};
