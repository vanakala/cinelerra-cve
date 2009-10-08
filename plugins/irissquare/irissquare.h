
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

#ifndef IRISSQUARE_H
#define IRISSQUARE_H

class IrisSquareMain;
class IrisSquareWindow;


#include "overlayframe.inc"
#include "pluginvclient.h"
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




class IrisSquareWindow : public BC_Window
{
public:
	IrisSquareWindow(IrisSquareMain *plugin, int x, int y);
	void create_objects();
	int close_event();
	IrisSquareMain *plugin;
	IrisSquareIn *in;
	IrisSquareOut *out;
};


PLUGIN_THREAD_HEADER(IrisSquareMain, IrisSquareThread, IrisSquareWindow)


class IrisSquareMain : public PluginVClient
{
public:
	IrisSquareMain(PluginServer *server);
	~IrisSquareMain();

// required for all realtime plugins
	void load_configuration();
	int process_realtime(VFrame *incoming, VFrame *outgoing);
	int load_defaults();
	int save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	int show_gui();
	void raise_window();
	int uses_gui();
	int is_transition();
	int is_video();
	char* plugin_title();
	int set_string();
	VFrame* new_picon();

	int direction;
	IrisSquareThread *thread;
	BC_Hash *defaults;
};

#endif
