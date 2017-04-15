
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
#define PLUGIN_IS_SYNTHESIS
#define PLUGIN_CUSTOM_LOAD_CONFIGURATION

// Old name was "Loop video"
#define PLUGIN_TITLE N_("Loop")
#define PLUGIN_CLASS LoopVideo
#define PLUGIN_CONFIG_CLASS LoopVideoConfig
#define PLUGIN_THREAD_CLASS LoopVideoThread
#define PLUGIN_GUI_CLASS LoopVideoWindow

#include "pluginmacros.h"

#include "bchash.h"
#include "bctitle.h"
#include "clip.h"
#include "filexml.h"
#include "language.h"
#include "pluginvclient.h"
#include "pluginwindow.h"
#include "picon_png.h"

#include <string.h>

class LoopVideo;

class LoopVideoConfig
{
public:
	LoopVideoConfig();

	ptstime duration;
	PLUGIN_CONFIG_CLASS_MEMBERS
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

class LoopVideoWindow : public PluginWindow
{
public:
	LoopVideoWindow(LoopVideo *plugin, int x, int y);
	~LoopVideoWindow();

	void update();

	LoopVideoFrames *frames;
	PLUGIN_GUI_CLASS_MEMBERS
};

PLUGIN_THREAD_HEADER

class LoopVideo : public PluginVClient
{
public:
	LoopVideo(PluginServer *server);
	~LoopVideo();

	PLUGIN_CLASS_MEMBERS

	void load_defaults();
	void save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);

	void process_frame(VFrame *frame);
};


REGISTER_PLUGIN

LoopVideoConfig::LoopVideoConfig()
{
	duration = 1.0;
}

LoopVideoWindow::LoopVideoWindow(LoopVideo *plugin, int x, int y)
 : PluginWindow(plugin->gui_string, 
	x,
	y, 
	210, 
	160)
{
	x = y = 10;

	add_subwindow(new BC_Title(x, y, _("Loop duration:")));
	y += 20;
	add_subwindow(frames = new LoopVideoFrames(plugin, 
		x, 
		y));
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

LoopVideoWindow::~LoopVideoWindow()
{
}

void LoopVideoWindow::update()
{
	frames->update((float)plugin->config.duration);
}


PLUGIN_THREAD_METHODS

LoopVideoFrames::LoopVideoFrames(LoopVideo *plugin, 
	int x, 
	int y)
 : BC_TextBox(x, 
	y, 
	100,
	1,
	(float)plugin->config.duration)
{
	this->plugin = plugin;
}

int LoopVideoFrames::handle_event()
{
	plugin->config.duration = atof(get_text());
	if(plugin->config.duration < EPSILON)
		plugin->config.duration = 1 / plugin->project_frame_rate;
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

PLUGIN_CLASS_METHODS

void LoopVideo::process_frame(VFrame *frame)
{
	ptstime current_loop_pts;
	ptstime start_pts = frame->get_pts();
// Truncate to next keyframe
// Get start of current loop
	KeyFrame *prev_keyframe = prev_keyframe_pts(start_pts);
	ptstime prev_pts = prev_keyframe->pos_time;
	if(prev_pts < EPSILON)
		prev_pts = source_start_pts;
	read_data(prev_keyframe);

// Get start of fragment in current loop
	current_loop_pts = prev_pts +
		fmod(start_pts - prev_pts, config.duration);

	frame->set_pts(current_loop_pts);
	get_frame(frame);
	frame->set_pts(start_pts);
}

int LoopVideo::load_configuration()
{
	KeyFrame *prev_keyframe;
	ptstime old_duration = config.duration;

	prev_keyframe = prev_keyframe_pts(source_pts);
	read_data(prev_keyframe);
	if(config.duration < EPSILON)
		config.duration = 1 / project_frame_rate;
	return PTSEQU(old_duration, config.duration);
}

void LoopVideo::load_defaults()
{
	framenum frames;
	defaults = load_defaults_file("loopvideo.rc");

	frames = defaults->get("FRAMES", 0);
	if(frames > 0)
		config.duration = frames / project_frame_rate;
	config.duration = defaults->get("DURATION", config.duration);
}

void LoopVideo::save_defaults()
{
	defaults->update("DURATION", config.duration);
	defaults->save();
}

void LoopVideo::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_string(keyframe->data, MESSAGESIZE);
	output.tag.set_title("LOOPVIDEO");
	output.tag.set_property("DURATION", config.duration);
	output.append_tag();
	output.tag.set_title("/LOOPVIDEO");
	output.append_tag();
	output.terminate_string();
}

void LoopVideo::read_data(KeyFrame *keyframe)
{
	FileXML input;
	framenum frames;

	input.set_shared_string(keyframe->data, strlen(keyframe->data));

	while(!input.read_tag())
	{
		if(input.tag.title_is("LOOPVIDEO"))
		{
			frames = input.tag.get_property("FRAMES", 0);
			if(frames > 0)
				config.duration = frames / project_frame_rate;
			config.duration = input.tag.get_property("DURATION", config.duration);
		}
	}
}
