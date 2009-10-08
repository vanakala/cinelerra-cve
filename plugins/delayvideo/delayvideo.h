
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

#ifndef DELAYVIDEO_H
#define DELAYVIDEO_H



#include "bchash.inc"
#include "guicast.h"
#include "mutex.h"
#include "pluginvclient.h"
#include "vframe.inc"



class DelayVideo;





class DelayVideoConfig
{
public:
	DelayVideoConfig();
	
	int equivalent(DelayVideoConfig &that);
	void copy_from(DelayVideoConfig &that);
	void interpolate(DelayVideoConfig &prev, 
		DelayVideoConfig &next, 
		int64_t prev_frame, 
		int64_t next_frame, 
		int64_t current_frame);

	// kjb - match defined update() type of float instead of double.
	float length;
};


class DelayVideoSlider : public BC_TextBox
{
public:
	DelayVideoSlider(DelayVideo *plugin, int x, int y);
	
	int handle_event();
	
	DelayVideo *plugin;
	
};


class DelayVideoWindow : public BC_Window
{
public:
	DelayVideoWindow(DelayVideo *plugin, int x, int y);
	~DelayVideoWindow();
	
	void create_objects();
	int close_event();
	void update_gui();
	
	DelayVideo *plugin;
	DelayVideoSlider *slider;
};


PLUGIN_THREAD_HEADER(DelayVideo, DelayVideoThread, DelayVideoWindow)




class DelayVideo : public PluginVClient
{
public:
	DelayVideo(PluginServer *server);
	~DelayVideo();
	
	int process_realtime(VFrame *input_ptr, VFrame *output_ptr);
	int is_realtime();
	char* plugin_title();
	int show_gui();
	void raise_window();
	int set_string();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	VFrame* new_picon();
	void reset();
	void reconfigure();



	int load_configuration();
	int load_defaults();
	int save_defaults();
	void update_gui();


	int need_reconfigure;
	int allocation;
	DelayVideoConfig config;
	DelayVideoThread *thread;
	VFrame **buffer;
	BC_Hash *defaults;
	VFrame *input, *output;
};



#endif
