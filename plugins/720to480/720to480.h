
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

#ifndef _720TO480_H
#define _720TO480_H

#define PLUGIN_IS_VIDEO
#define PLUGIN_CUSTOM_LOAD_CONFIGURATION

#define PLUGIN_TITLE N_("720 to 480")
#define PLUGIN_CLASS _720to480Main
#define PLUGIN_CONFIG_CLASS _720to480Config
#define PLUGIN_GUI_CLASS _720to480Window

#include "pluginmacros.h"

#include "language.h"
#include "pluginvclient.h"
#include "pluginwindow.h"

class _720to480Main;
class _720to480Order;

class _720to480Window : public PluginWindow
{
public:
	_720to480Window(_720to480Main *plugin, int x, int y);
	~_720to480Window();

	void set_first_field(int first_field);

	_720to480Order *odd_first;
	_720to480Order *even_first;
	PLUGIN_GUI_CLASS_MEMBERS
};


class _720to480Order : public BC_Radial
{
public:
	_720to480Order(_720to480Main *client, 
		_720to480Window *window, 
		int output, 
		int x, 
		int y, 
		char *text);
	int handle_event();

	_720to480Main *client;
	_720to480Window *window;
	int output;
};

class _720to480Config
{
public:
	_720to480Config();

	int first_field;
	PLUGIN_CONFIG_CLASS_MEMBERS
};

class _720to480Main : public PluginVClient
{
public:
	_720to480Main(PluginServer *server);
	~_720to480Main();
	PLUGIN_CLASS_MEMBERS

	void load_defaults();
	void save_defaults();
	double get_framerate();
	void init_plugin();
	int process_loop(VFrame *output);

	void reduce_field(VFrame *output, VFrame *input, int dest_row);

	VFrame *temp;
	ptstime input_pts;
};

#endif
