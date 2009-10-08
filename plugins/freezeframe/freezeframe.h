
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

#ifndef FREEZEFRAME_H
#define FREEZEFRAME_H




#include "filexml.inc"
#include "mutex.h"
#include "pluginvclient.h"



class FreezeFrameWindow;
class FreezeFrameMain;
class FreezeFrameThread;

class FreezeFrameConfig
{
public:
	FreezeFrameConfig();
	void copy_from(FreezeFrameConfig &that);
	int equivalent(FreezeFrameConfig &that);
	void interpolate(FreezeFrameConfig &prev, 
		FreezeFrameConfig &next, 
		long prev_frame, 
		long next_frame, 
		long current_frame);
	int enabled;
	int line_double;
};

class FreezeFrameToggle : public BC_CheckBox
{
public:
	FreezeFrameToggle(FreezeFrameMain *client, 
		int *value, 
		int x, 
		int y,
		char *text);
	~FreezeFrameToggle();
	int handle_event();
	FreezeFrameMain *client;
	int *value;
};

class FreezeFrameWindow : public BC_Window
{
public:
	FreezeFrameWindow(FreezeFrameMain *client, int x, int y);
	~FreezeFrameWindow();
	
	int create_objects();
	int close_event();
	
	FreezeFrameMain *client;
	FreezeFrameToggle *enabled;
//	FreezeFrameToggle *line_double;
};

PLUGIN_THREAD_HEADER(FreezeFrameMain, FreezeFrameThread, FreezeFrameWindow)

class FreezeFrameMain : public PluginVClient
{
public:
	FreezeFrameMain(PluginServer *server);
	~FreezeFrameMain();

	PLUGIN_CLASS_MEMBERS(FreezeFrameConfig, FreezeFrameThread)

	int process_buffer(VFrame *frame,
		int64_t start_position,
		double frame_rate);
	int is_realtime();
	void update_gui();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	int load_defaults();
	int save_defaults();
	int is_synthesis();
	int handle_opengl();


// Frame to replicate
	VFrame *first_frame;
// Position of frame to replicate
	int64_t first_frame_position;
};


#endif
