
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

#ifndef SWAPCHANNELS_H
#define SWAPCHANNELS_H

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME
#define PLUGIN_IS_SYNTHESIS
#define PLUGIN_CUSTOM_LOAD_CONFIGURATION

#define PLUGIN_TITLE N_("Swap channels")
#define PLUGIN_CLASS SwapMain
#define PLUGIN_CONFIG_CLASS SwapConfig
#define PLUGIN_THREAD_CLASS SwapThread
#define PLUGIN_GUI_CLASS SwapWindow

#include "pluginmacros.h"

#include "bchash.inc"
#include "guicast.h"
#include "language.h"
#include "mutex.h"
#include "pluginvclient.h"
#include "vframe.inc"


#define RED_SRC 0
#define GREEN_SRC 1
#define BLUE_SRC 2
#define ALPHA_SRC 3
#define NO_SRC 4
#define MAX_SRC 5


class SwapConfig
{
public:
	SwapConfig();

	int equivalent(SwapConfig &that);
	void copy_from(SwapConfig &that);

	int red;
	int green;
	int blue;
	int alpha;
	PLUGIN_CONFIG_CLASS_MEMBERS
};


class SwapMenu : public BC_PopupMenu
{
public:
	SwapMenu(SwapMain *client, int *output, int x, int y);

	int handle_event();
	void create_objects();

	SwapMain *client;
	int *output;
};


class SwapItem : public BC_MenuItem
{
public:
	SwapItem(SwapMenu *menu, const char *title);

	int handle_event();

	SwapMenu *menu;
	char *title;
};

class SwapWindow : public BC_Window
{
public:
	SwapWindow(SwapMain *plugin, int x, int y);
	~SwapWindow();

	void update();

	SwapMenu *red;
	SwapMenu *green;
	SwapMenu *blue;
	SwapMenu *alpha;
	PLUGIN_GUI_CLASS_MEMBERS
};


PLUGIN_THREAD_HEADER

class SwapMain : public PluginVClient
{
public:
	SwapMain(PluginServer *server);
	~SwapMain();

	PLUGIN_CLASS_MEMBERS

// required for all realtime plugins
	void process_realtime(VFrame *input_ptr, VFrame *output_ptr);

	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);

	void load_defaults();
	void save_defaults();

// parameters needed for processor
	const char* output_to_text(int value);
	int text_to_output(const char *text);

	VFrame *temp;
};

#endif
