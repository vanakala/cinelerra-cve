
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

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME
#define PLUGIN_CUSTOM_LOAD_CONFIGURATION

// Old name was "Reverse video"
#define PLUGIN_TITLE N_("Reverse")
#define PLUGIN_CLASS ReverseVideo
#define PLUGIN_CONFIG_CLASS ReverseVideoConfig
#define PLUGIN_THREAD_CLASS ReverseVideoThread
#define PLUGIN_GUI_CLASS ReverseVideoWindow

#include "pluginmacros.h"

#include "bchash.h"
#include "filexml.h"
#include "language.h"
#include "pluginvclient.h"
#include "pluginwindow.h"

#include <string.h>
#include "picon_png.h"

class ReverseVideoConfig
{
public:
	ReverseVideoConfig();
	int enabled;
	PLUGIN_CONFIG_CLASS_MEMBERS
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


class ReverseVideoWindow : public PluginWindow
{
public:
	ReverseVideoWindow(ReverseVideo *plugin, int x, int y);
	~ReverseVideoWindow();

	void update();

	ReverseVideoEnabled *enabled;
	PLUGIN_GUI_CLASS_MEMBERS
};

PLUGIN_THREAD_HEADER

class ReverseVideo : public PluginVClient
{
public:
	ReverseVideo(PluginServer *server);
	~ReverseVideo();

	PLUGIN_CLASS_MEMBERS

	void load_defaults();
	void save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);

	void process_frame(VFrame *frame);

	ptstime input_pts;
};


REGISTER_PLUGIN

ReverseVideoConfig::ReverseVideoConfig()
{
	enabled = 1;
}

PLUGIN_THREAD_METHODS

ReverseVideoWindow::ReverseVideoWindow(ReverseVideo *plugin, int x, int y)
 : PluginWindow(plugin->gui_string, 
	x,
	y, 
	210, 
	160)
{
	x = y = 10;

	add_subwindow(enabled = new ReverseVideoEnabled(plugin, 
		x, 
		y));
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

ReverseVideoWindow::~ReverseVideoWindow()
{
}

void ReverseVideoWindow::update()
{
	enabled->update(plugin->config.enabled);
}


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

PLUGIN_CLASS_METHODS

void ReverseVideo::process_frame(VFrame *frame)
{
	load_configuration();

	if(config.enabled)
	{
		ptstime cpts = frame->get_pts();
		frame->set_pts(input_pts);
		get_frame(frame);
		frame->set_pts(cpts);
	}
	else
		get_frame(frame);
}

int ReverseVideo::load_configuration()
{
	KeyFrame *prev_keyframe, *next_keyframe;
	next_keyframe = next_keyframe_pts(source_pts);
	prev_keyframe = prev_keyframe_pts(source_pts);

// Previous keyframe stays in config object.
	read_data(prev_keyframe);

	ptstime prev_pts = prev_keyframe->pos_time;
	ptstime next_pts = next_keyframe->pos_time;

	if(prev_pts < EPSILON && next_pts < EPSILON)
	{
		next_pts = prev_pts = source_start_pts;
	}

	ptstime range_start_pts = prev_pts;
	ptstime range_end_pts = next_pts;

// Between keyframe and edge of range or no keyframes
	if(PTSEQU(range_start_pts, range_end_pts))
	{
// Between first keyframe and start of effect
		if(source_pts >= source_start_pts &&
			source_pts < range_start_pts)
		{
			range_start_pts = source_start_pts;
		}
		else
// Between last keyframe and end of effect
		if(source_pts >= range_start_pts &&
			source_pts < source_start_pts + total_len_pts)
		{
			range_end_pts = source_start_pts + total_len_pts;
		}
		else
		{
// Should never get here
			;
		}
	}

	input_pts = source_pts - range_start_pts;
	input_pts = range_end_pts - input_pts - 1 / project_frame_rate;
	return 1;
}

void ReverseVideo::load_defaults()
{
	defaults = load_defaults_file("reversevideo.rc");

	config.enabled = defaults->get("ENABLED", config.enabled);
}

void ReverseVideo::save_defaults()
{
	defaults->update("ENABLED", config.enabled);
	defaults->save();
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
}

void ReverseVideo::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->data, strlen(keyframe->data));

	while(!input.read_tag())
	{
		if(input.tag.title_is("REVERSEVIDEO"))
		{
			config.enabled = input.tag.get_property("ENABLED", config.enabled);
		}
	}
}
