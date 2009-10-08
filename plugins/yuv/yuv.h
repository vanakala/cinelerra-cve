
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

#ifndef YUV_H
#define YUV_H

#define MAXVALUE 100

class YUVMain;
class YUVEngine;

#include "bcbase.h"
#include "../colors/colors.h"
#include "yuvwindow.h"
#include "pluginvclient.h"


class YUVMain : public PluginVClient
{
public:
	YUVMain(int argc, char *argv[]);
	~YUVMain();

// required for all realtime plugins
	int process_realtime(long size, VFrame **input_ptr, VFrame **output_ptr);
	int plugin_is_realtime();
	int plugin_is_multi_channel();
	char* plugin_title();
	int start_gui();
	int stop_gui();
	int show_gui();
	int hide_gui();
	int set_string();
	int load_defaults();
	int save_defaults();
	int save_data(char *text);
	int read_data(char *text);

// parameters needed
	int reconfigure();    // Rebuild tables
	int y;
	int u;
	int v;
	int automated_function;
	int reconfigure_flag;

// a thread for the GUI
	YUVThread *thread;

private:
	BC_Hash *defaults;
	YUVEngine **engine;
};

class YUVEngine : public Thread
{
public:
	YUVEngine(YUVMain *plugin, int start_y, int end_y);
	~YUVEngine();
	
	int start_process_frame(VFrame **output, VFrame **input, int size);
	int wait_process_frame();
	void run();
	
	YUVMain *plugin;
	int start_y;
	int end_y;
	int size;
	VFrame **output, **input;
	int last_frame;
	Mutex input_lock, output_lock;
    YUV yuv;
};


#endif
