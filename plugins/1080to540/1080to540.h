
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

#ifndef _1080TO540_H
#define _1080TO540_H

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME
#define PLUGIN_USES_TMPFRAME

#define PLUGIN_TITLE N_("1080 to 540")
#define PLUGIN_CLASS _1080to540Main 
#define PLUGIN_CONFIG_CLASS _1080to540Config
#define PLUGIN_THREAD_CLASS _1080to540Thread
#define PLUGIN_GUI_CLASS _1080to540Window

#include "pluginmacros.h"

#include "language.h"
#include "pluginvclient.h"
#include "pluginwindow.h"

class _1080to540Option;

class _1080to540Window : public PluginWindow
{
public:
	_1080to540Window(_1080to540Main *plugin, int x, int y);

	int set_first_field(int first_field, int send_event);
	void update();

	_1080to540Option *odd_first;
	_1080to540Option *even_first;
	PLUGIN_GUI_CLASS_MEMBERS
};

PLUGIN_THREAD_HEADER

class _1080to540Option : public BC_Radial
{
public:
	_1080to540Option(_1080to540Main *client,
		_1080to540Window *window,
		int output, 
		int x, 
		int y, 
		char *text);
	int handle_event();

	_1080to540Main *client;
	_1080to540Window *window;
	int output;
};

class _1080to540Config
{
public:
	_1080to540Config();

	int equivalent(_1080to540Config &that);
	void copy_from(_1080to540Config &that);
	void interpolate(_1080to540Config &prev, 
		_1080to540Config &next, 
		ptstime prev_pts,
		ptstime next_pts,
		ptstime current_pts);

	int first_field;
	PLUGIN_CONFIG_CLASS_MEMBERS
};

class _1080to540Main : public PluginVClient
{
public:
	_1080to540Main(PluginServer *server);
	~_1080to540Main();

	PLUGIN_CLASS_MEMBERS

	VFrame *process_tmpframe(VFrame *input);

	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void load_defaults();
	void save_defaults();

	void reduce_field(VFrame *output, VFrame *input, int src_field, int dst_field);
};

#endif
