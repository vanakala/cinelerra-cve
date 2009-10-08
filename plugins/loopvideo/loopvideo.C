
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

#include "bcdisplayinfo.h"
#include "clip.h"
#include "bchash.h"
#include "filexml.h"
#include "guicast.h"
#include "language.h"
#include "pluginvclient.h"
#include "transportque.h"

#include <string.h>

class LoopVideo;

class LoopVideoConfig
{
public:
	LoopVideoConfig();
	int64_t frames;
};


class LoopVideoFrames : public BC_TextBox
{
public:
	LoopVideoFrames(LoopVideo *plugin,
		int x,
		int y);
	int handle_event();
	LoopVideo *plugin;
};

class LoopVideoWindow : public BC_Window
{
public:
	LoopVideoWindow(LoopVideo *plugin, int x, int y);
	~LoopVideoWindow();
	void create_objects();
	int close_event();
	LoopVideo *plugin;
	LoopVideoFrames *frames;
};

PLUGIN_THREAD_HEADER(LoopVideo, LoopVideoThread, LoopVideoWindow)

class LoopVideo : public PluginVClient
{
public:
	LoopVideo(PluginServer *server);
	~LoopVideo();

	PLUGIN_CLASS_MEMBERS(LoopVideoConfig, LoopVideoThread)

	int load_defaults();
	int save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void update_gui();
	int is_realtime();
	int is_synthesis();
	int process_buffer(VFrame *frame,
		int64_t start_position,
		double frame_rate);
};







REGISTER_PLUGIN(LoopVideo);



LoopVideoConfig::LoopVideoConfig()
{
	frames = 30;
}





LoopVideoWindow::LoopVideoWindow(LoopVideo *plugin, int x, int y)
 : BC_Window(plugin->gui_string, 
 	x, 
	y, 
	210, 
	160, 
	200, 
	160, 
	0, 
	0,
	1)
{
	this->plugin = plugin;
}

LoopVideoWindow::~LoopVideoWindow()
{
}

void LoopVideoWindow::create_objects()
{
	int x = 10, y = 10;

	add_subwindow(new BC_Title(x, y, _("Frames to loop:")));
	y += 20;
	add_subwindow(frames = new LoopVideoFrames(plugin, 
		x, 
		y));
	show_window();
	flush();
}

WINDOW_CLOSE_EVENT(LoopVideoWindow)


PLUGIN_THREAD_OBJECT(LoopVideo, LoopVideoThread, LoopVideoWindow)






LoopVideoFrames::LoopVideoFrames(LoopVideo *plugin, 
	int x, 
	int y)
 : BC_TextBox(x, 
	y, 
	100,
	1,
	plugin->config.frames)
{
	this->plugin = plugin;
}

int LoopVideoFrames::handle_event()
{
	plugin->config.frames = atol(get_text());
	plugin->config.frames = MAX(1, plugin->config.frames);
	plugin->send_configure_change();
	return 1;
}









LoopVideo::LoopVideo(PluginServer *server)
 : PluginVClient(server)
{
	PLUGIN_CONSTRUCTOR_MACRO
}


LoopVideo::~LoopVideo()
{
	PLUGIN_DESTRUCTOR_MACRO
}

char* LoopVideo::plugin_title() { return N_("Loop video"); }
int LoopVideo::is_realtime() { return 1; }
int LoopVideo::is_synthesis() { return 1; }

#include "picon_png.h"
NEW_PICON_MACRO(LoopVideo)

SHOW_GUI_MACRO(LoopVideo, LoopVideoThread)

RAISE_WINDOW_MACRO(LoopVideo)

SET_STRING_MACRO(LoopVideo);


int LoopVideo::process_buffer(VFrame *frame,
		int64_t start_position,
		double frame_rate)
{
	int64_t current_loop_position;

// Truncate to next keyframe
	if(get_direction() == PLAY_FORWARD)
	{
// Get start of current loop
		KeyFrame *prev_keyframe = get_prev_keyframe(start_position);
		int64_t prev_position = edl_to_local(prev_keyframe->position);
		if(prev_position == 0)
			prev_position = get_source_start();
		read_data(prev_keyframe);

// Get start of fragment in current loop
		current_loop_position = prev_position +
			((start_position - prev_position) % 
				config.frames);
		while(current_loop_position < prev_position) current_loop_position += config.frames;
		while(current_loop_position >= prev_position + config.frames) current_loop_position -= config.frames;
	}
	else
	{
		KeyFrame *prev_keyframe = get_next_keyframe(start_position);
		int64_t prev_position = edl_to_local(prev_keyframe->position);
		if(prev_position == 0)
			prev_position = get_source_start() + get_total_len();
		read_data(prev_keyframe);

		current_loop_position = prev_position - 
			((prev_position - start_position) %
				  config.frames);
		while(current_loop_position <= prev_position - config.frames) current_loop_position += config.frames;
		while(current_loop_position > prev_position) current_loop_position -= config.frames;
	}


// printf("LoopVideo::process_buffer 100 %lld %lld %lld %d\n", 
// current_position, current_loop_position, current_loop_end, fragment_size);
	read_frame(frame,
		0,
		current_loop_position,
		frame_rate);

	return 0;
}




int LoopVideo::load_configuration()
{
	KeyFrame *prev_keyframe;
	int64_t old_frames = config.frames;
	prev_keyframe = get_prev_keyframe(get_source_position());
	read_data(prev_keyframe);
	config.frames = MAX(config.frames, 1);
	return old_frames != config.frames;
}

int LoopVideo::load_defaults()
{
	char directory[BCTEXTLEN];
// set the default directory
	sprintf(directory, "%sloopaudio.rc", BCASTDIR);

// load the defaults
	defaults = new BC_Hash(directory);
	defaults->load();

	config.frames = defaults->get("FRAMES", config.frames);
	return 0;
}

int LoopVideo::save_defaults()
{
	defaults->update("FRAMES", config.frames);
	defaults->save();
	return 0;
}

void LoopVideo::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_string(keyframe->data, MESSAGESIZE);
	output.tag.set_title("LOOPVIDEO");
	output.tag.set_property("FRAMES", config.frames);
	output.append_tag();
	output.tag.set_title("/LOOPVIDEO");
	output.append_tag();
	output.terminate_string();
}

void LoopVideo::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->data, strlen(keyframe->data));

	int result = 0;

	while(!input.read_tag())
	{
		if(input.tag.title_is("LOOPVIDEO"))
		{
			config.frames = input.tag.get_property("FRAMES", config.frames);
		}
	}
}

void LoopVideo::update_gui()
{
	if(thread)
	{
		load_configuration();
		thread->window->lock_window();
		thread->window->frames->update(config.frames);
		thread->window->unlock_window();
	}
}





