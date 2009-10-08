
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
#include "bchash.h"
#include "filexml.h"
#include "guicast.h"
#include "language.h"
#include "pluginvclient.h"
#include "transportque.h"

#include <string.h>

class ReverseVideo;

class ReverseVideoConfig
{
public:
	ReverseVideoConfig();
	int enabled;
};


class ReverseVideoEnabled : public BC_CheckBox
{
public:
	ReverseVideoEnabled(ReverseVideo *plugin,
		int x,
		int y);
	int handle_event();
	ReverseVideo *plugin;
};

class ReverseVideoWindow : public BC_Window
{
public:
	ReverseVideoWindow(ReverseVideo *plugin, int x, int y);
	~ReverseVideoWindow();
	void create_objects();
	int close_event();
	ReverseVideo *plugin;
	ReverseVideoEnabled *enabled;
};

PLUGIN_THREAD_HEADER(ReverseVideo, ReverseVideoThread, ReverseVideoWindow)

class ReverseVideo : public PluginVClient
{
public:
	ReverseVideo(PluginServer *server);
	~ReverseVideo();

	PLUGIN_CLASS_MEMBERS(ReverseVideoConfig, ReverseVideoThread)

	int load_defaults();
	int save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void update_gui();
	int is_realtime();
	int process_buffer(VFrame *frame,
			int64_t start_position,
			double frame_rate);

	int64_t input_position;
};







REGISTER_PLUGIN(ReverseVideo);



ReverseVideoConfig::ReverseVideoConfig()
{
	enabled = 1;
}





ReverseVideoWindow::ReverseVideoWindow(ReverseVideo *plugin, int x, int y)
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

ReverseVideoWindow::~ReverseVideoWindow()
{
}

void ReverseVideoWindow::create_objects()
{
	int x = 10, y = 10;

	add_subwindow(enabled = new ReverseVideoEnabled(plugin, 
		x, 
		y));
	show_window();
	flush();
}

WINDOW_CLOSE_EVENT(ReverseVideoWindow)


PLUGIN_THREAD_OBJECT(ReverseVideo, ReverseVideoThread, ReverseVideoWindow)






ReverseVideoEnabled::ReverseVideoEnabled(ReverseVideo *plugin, 
	int x, 
	int y)
 : BC_CheckBox(x, 
	y, 
	plugin->config.enabled,
	_("Enabled"))
{
	this->plugin = plugin;
}

int ReverseVideoEnabled::handle_event()
{
	plugin->config.enabled = get_value();
	plugin->send_configure_change();
	return 1;
}









ReverseVideo::ReverseVideo(PluginServer *server)
 : PluginVClient(server)
{
	PLUGIN_CONSTRUCTOR_MACRO
}


ReverseVideo::~ReverseVideo()
{
	PLUGIN_DESTRUCTOR_MACRO
}

char* ReverseVideo::plugin_title() { return N_("Reverse video"); }
int ReverseVideo::is_realtime() { return 1; }

#include "picon_png.h"
NEW_PICON_MACRO(ReverseVideo)

SHOW_GUI_MACRO(ReverseVideo, ReverseVideoThread)

RAISE_WINDOW_MACRO(ReverseVideo)

SET_STRING_MACRO(ReverseVideo);


int ReverseVideo::process_buffer(VFrame *frame,
		int64_t start_position,
		double frame_rate)
{
	load_configuration();

	if(config.enabled)
		read_frame(frame,
			0,
			input_position,
			frame_rate);
	else
		read_frame(frame,
			0,
			start_position,
			frame_rate);
	return 0;
}




int ReverseVideo::load_configuration()
{
	KeyFrame *prev_keyframe, *next_keyframe;
	next_keyframe = get_next_keyframe(get_source_position());
	prev_keyframe = get_prev_keyframe(get_source_position());
// Previous keyframe stays in config object.
	read_data(prev_keyframe);

	int64_t prev_position = edl_to_local(prev_keyframe->position);
	int64_t next_position = edl_to_local(next_keyframe->position);

	if(prev_position == 0 && next_position == 0) 
	{
		next_position = prev_position = get_source_start();
	}

// Get range to flip in requested rate
	int64_t range_start = prev_position;
	int64_t range_end = next_position;

// Between keyframe and edge of range or no keyframes
	if(range_start == range_end)
	{
// Between first keyframe and start of effect
		if(get_source_position() >= get_source_start() &&
			get_source_position() < range_start)
		{
			range_start = get_source_start();
		}
		else
// Between last keyframe and end of effect
		if(get_source_position() >= range_start &&
			get_source_position() < get_source_start() + get_total_len())
		{
			range_end = get_source_start() + get_total_len();
		}
		else
		{
// Should never get here
			;
		}
	}


// Convert start position to new direction
	if(get_direction() == PLAY_FORWARD)
	{
		input_position = get_source_position() - range_start;
		input_position = range_end - input_position - 1;
	}
	else
	{
		input_position = range_end - get_source_position();
		input_position = range_start + input_position + 1;
	}
// printf("ReverseVideo::load_configuration 2 start=%lld end=%lld current=%lld input=%lld\n", 
// range_start, 
// range_end, 
// get_source_position(),
// input_position);

	return 0;
}

int ReverseVideo::load_defaults()
{
	char directory[BCTEXTLEN];
// set the default directory
	sprintf(directory, "%sreversevideo.rc", BCASTDIR);

// load the defaults
	defaults = new BC_Hash(directory);
	defaults->load();

	config.enabled = defaults->get("ENABLED", config.enabled);
	return 0;
}

int ReverseVideo::save_defaults()
{
	defaults->update("ENABLED", config.enabled);
	defaults->save();
	return 0;
}

void ReverseVideo::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_string(keyframe->data, MESSAGESIZE);
	output.tag.set_title("REVERSEVIDEO");
	output.tag.set_property("ENABLED", config.enabled);
	output.append_tag();
	output.tag.set_title("/REVERSEVIDEO");
	output.append_tag();
	output.terminate_string();
}

void ReverseVideo::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->data, strlen(keyframe->data));

	int result = 0;

	while(!input.read_tag())
	{
		if(input.tag.title_is("REVERSEVIDEO"))
		{
			config.enabled = input.tag.get_property("ENABLED", config.enabled);
		}
	}
}

void ReverseVideo::update_gui()
{
	if(thread)
	{
		load_configuration();
		thread->window->lock_window();
		thread->window->enabled->update(config.enabled);
		thread->window->unlock_window();
	}
}





