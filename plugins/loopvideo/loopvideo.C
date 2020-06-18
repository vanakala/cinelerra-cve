
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
	ptstime start_pts = frame->get_pts();
// Truncate to next keyframe
// Get start of current loop
	KeyFrame *prev_keyframe = prev_keyframe_pts(start_pts);
	ptstime prev_pts = prev_keyframe->pos_time;
	if(prev_pts < EPSILON)
		prev_pts = get_start();
	read_data(prev_keyframe);

// Get start of fragment in current loop
	current_loop_pts = prev_pts +
		fmod(start_pts - prev_pts, config.duration);

	frame->set_pts(current_loop_pts);
	get_frame(frame);
	frame->set_pts(start_pts);
	return frame;
}

int LoopVideo::load_configuration()
{
	KeyFrame *prev_keyframe;
	ptstime old_duration = config.duration;

	if(!(prev_keyframe = prev_keyframe_pts(source_pts)))
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
