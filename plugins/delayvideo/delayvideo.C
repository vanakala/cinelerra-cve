// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bctitle.h"
#include "clip.h"
#include "bchash.h"
#include "delayvideo.h"
#include "filexml.h"
#include "language.h"
#include "picon_png.h"
#include "vframe.h"

#include <string.h>


REGISTER_PLUGIN


DelayVideoConfig::DelayVideoConfig()
{
	length = 0.5;
}

int DelayVideoConfig::equivalent(DelayVideoConfig &that)
{
	return EQUIV(length, that.length);
}

void DelayVideoConfig::copy_from(DelayVideoConfig &that)
{
	length = that.length;
}

void DelayVideoConfig::interpolate(DelayVideoConfig &prev, 
		DelayVideoConfig &next, 
		ptstime prev_pts,
		ptstime next_pts,
		ptstime current_pts)
{
	this->length = prev.length;
}


DelayVideoWindow::DelayVideoWindow(DelayVideo *plugin, int x, int y)
 : PluginWindow(plugin->gui_string, 
	x,
	y,
	210, 
	120)
{
	x = y = 10;

	add_subwindow(new BC_Title(x, y, _("Delay seconds:")));
	y += 20;
	add_subwindow(slider = new DelayVideoSlider(plugin, x, y));
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

void DelayVideoWindow::update()
{
	slider->update(plugin->config.length);
}


DelayVideoSlider::DelayVideoSlider(DelayVideo *plugin, int x, int y)
 : BC_TextBox(x, y, 150, 1, plugin->config.length)
{
	this->plugin = plugin;
}

int DelayVideoSlider::handle_event()
{
	plugin->config.length = atof(get_text());
	CLAMP(plugin->config.length, 0, 10);
	plugin->send_configure_change();
	return 1;
}


PLUGIN_THREAD_METHODS


DelayVideo::DelayVideo(PluginServer *server)
 : PluginVClient(server)
{
	PLUGIN_CONSTRUCTOR_MACRO
}

DelayVideo::~DelayVideo()
{
	PLUGIN_DESTRUCTOR_MACRO
}

PLUGIN_CLASS_METHODS


VFrame *DelayVideo::process_tmpframe(VFrame *frame)
{
	ptstime pts = frame->get_pts();

	if(load_configuration())
		update_gui();

	if(config.length < pts)
	{
		frame->set_pts(pts - config.length);
		frame = get_frame(frame);
		frame->set_pts(pts);
	}
	return frame;
}

void DelayVideo::save_data(KeyFrame *keyframe)
{
	FileXML output;

	output.tag.set_title("DELAYVIDEO");
	output.tag.set_property("LENGTH", config.length);
	output.append_tag();
	output.tag.set_title("/DELAYVIDEO");
	output.append_tag();
	keyframe->set_data(output.string);

}

void DelayVideo::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());

	while(!input.read_tag())
	{
		if(input.tag.title_is("DELAYVIDEO"))
		{
			config.length = input.tag.get_property("LENGTH", config.length);
		}
	}
}

void DelayVideo::load_defaults()
{
	defaults = load_defaults_file("delayvideo.rc");
	config.length = defaults->get("LENGTH", config.length);
}

void DelayVideo::save_defaults()
{
	defaults->update("LENGTH", config.length);
	defaults->save();
}
