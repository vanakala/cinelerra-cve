
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


#include "overlayframe.inc"
#include "pluginvclient.h"

class _1080to540Main;
class _1080to540Option;

class _1080to540Window : public BC_Window
{
public:
	_1080to540Window(_1080to540Main *client, int x, int y);
	~_1080to540Window();
	
	int create_objects();
	int close_event();
	int set_first_field(int first_field, int send_event);
	
	_1080to540Main *client;
	_1080to540Option *odd_first;
	_1080to540Option *even_first;
};

PLUGIN_THREAD_HEADER(_1080to540Main, _1080to540Thread, _1080to540Window);

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
		long prev_frame, 
		long next_frame, 
		long current_frame);

	int first_field;
};

class _1080to540Main : public PluginVClient
{
public:
	_1080to540Main(PluginServer *server);
	~_1080to540Main();


	PLUGIN_CLASS_MEMBERS(_1080to540Config, _1080to540Thread)
	

// required for all realtime plugins
	int process_realtime(VFrame *input, VFrame *output);
	int is_realtime();
	int hide_gui();
	void update_gui();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	int load_defaults();
	int save_defaults();

	void reduce_field(VFrame *output, VFrame *input, int src_field, int dst_field);

	VFrame *temp;
};


#endif
