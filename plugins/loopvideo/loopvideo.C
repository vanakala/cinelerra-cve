// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bchash.h"
#include "bctitle.h"
#include "clip.h"
#include "filexml.h"
#include "loopvideo.h"
#include "pluginvclient.h"
#include "pluginwindow.h"
#include "picon_png.h"

#include <string.h>

REGISTER_PLUGIN

LoopVideoConfig::LoopVideoConfig()
{
	duration = 1.0;
}

LoopVideoWindow::LoopVideoWindow(LoopVideo *plugin, int x, int y)
 : PluginWindow(plugin,
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
	plugin->config.duration)
{
	this->plugin = plugin;
}

int LoopVideoFrames::handle_event()
{
	plugin->config.duration = atof(get_text());
	if(plugin->config.duration < EPSILON)
		plugin->config.duration = 1 / plugin->get_project_framerate();
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

VFrame *LoopVideo::process_tmpframe(VFrame *frame)
{
	ptstime current_loop_pts;
	ptstime frame_pts = frame->get_pts();
	ptstime start_pts = get_start();

	if(load_configuration())
		update_gui();

// Get start of fragment in current loop
	current_loop_pts = start_pts +
		fmod(frame_pts - start_pts, config.duration);

	frame->set_pts(current_loop_pts);
	frame = get_frame(frame);
	frame->set_pts(frame_pts);
	return frame;
}

int LoopVideo::load_configuration()
{
	KeyFrame *prev_keyframe = get_first_keyframe();
	ptstime old_duration = config.duration;

	if(!prev_keyframe)
		return 0;

	read_data(prev_keyframe);

	if(config.duration < EPSILON)
		config.duration = 1 / get_project_framerate();
	return PTSEQU(old_duration, config.duration);
}

void LoopVideo::load_defaults()
{
	framenum frames;
	defaults = load_defaults_file("loopvideo.rc");

	frames = defaults->get("FRAMES", 0);
	if(frames > 0)
		config.duration = frames / get_project_framerate();
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

	output.tag.set_title("LOOPVIDEO");
	output.tag.set_property("DURATION", config.duration);
	output.append_tag();
	output.tag.set_title("/LOOPVIDEO");
	output.append_tag();
	keyframe->set_data(output.string);
}

void LoopVideo::read_data(KeyFrame *keyframe)
{
	FileXML input;
	framenum frames;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());

	while(!input.read_tag())
	{
		if(input.tag.title_is("LOOPVIDEO"))
		{
			frames = input.tag.get_property("FRAMES", 0);
			if(frames > 0)
				config.duration = frames / get_project_framerate();
			config.duration = input.tag.get_property("DURATION", config.duration);
		}
	}
}
