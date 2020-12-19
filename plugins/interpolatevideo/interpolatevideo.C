// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bchash.h"
#include "bctitle.h"
#include "clip.h"
#include "filexml.h"
#include "interpolatevideo.h"
#include "keyframe.h"
#include "picon_png.h"
#include "vframe.h"

#include <string.h>
#include <stdint.h>


InterpolateVideoConfig::InterpolateVideoConfig()
{
	input_rate = (double)30000 / 1001;
	use_keyframes = 0;
}

void InterpolateVideoConfig::copy_from(InterpolateVideoConfig *config)
{
	input_rate = config->input_rate;
	use_keyframes = config->use_keyframes;
}

int InterpolateVideoConfig::equivalent(InterpolateVideoConfig *config)
{
	return (use_keyframes == config->use_keyframes) &&
		(!use_keyframes && EQUIV(input_rate, config->input_rate));
}


InterpolateVideoWindow::InterpolateVideoWindow(InterpolateVideo *plugin, int x, int y)
 : PluginWindow(plugin->gui_string, 
	x,
	y, 
	210, 
	160)
{
	BC_Title *title;
	x = y = 10;

	add_subwindow(title = new BC_Title(x, y, _("Input frames per second:")));
	y += 30;
	add_subwindow(rate = new InterpolateVideoRate(plugin, 
		this, 
		x, 
		y));
	rate->update(plugin->config.input_rate);
	y += 30;
	add_subwindow(keyframes = new InterpolateVideoKeyframes(plugin,
		this,
		x, 
		y));
	PLUGIN_GUI_CONSTRUCTOR_MACRO
	update_enabled();
}

void InterpolateVideoWindow::update()
{
	rate->update(plugin->config.input_rate);
	keyframes->update(plugin->config.use_keyframes);
	update_enabled();
}

void InterpolateVideoWindow::update_enabled()
{
	if(plugin->config.use_keyframes)
		rate->disable();
	else
		rate->enable();
}


InterpolateVideoRate::InterpolateVideoRate(InterpolateVideo *plugin, 
	InterpolateVideoWindow *gui,
	int x,
	int y)
 : FrameRateSelection(x, y, gui, &plugin->config.input_rate)
{
	this->plugin = plugin;
}

int InterpolateVideoRate::handle_event()
{
	int result;

	result = FrameRateSelection::handle_event();
	plugin->send_configure_change();
	return result;
}


InterpolateVideoKeyframes::InterpolateVideoKeyframes(InterpolateVideo *plugin,
	InterpolateVideoWindow *gui,
	int x, 
	int y)
 : BC_CheckBox(x, 
	y, 
	plugin->config.use_keyframes, 
	_("Use keyframes as input"))
{
	this->plugin = plugin;
	this->gui = gui;
}

int InterpolateVideoKeyframes::handle_event()
{
	plugin->config.use_keyframes = get_value();
	gui->update_enabled();
	plugin->send_configure_change();
	return 1;
}


PLUGIN_THREAD_METHODS
REGISTER_PLUGIN


InterpolateVideo::InterpolateVideo(PluginServer *server)
 : PluginVClient(server)
{
	frame1 = 0;
	frame2 = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}


InterpolateVideo::~InterpolateVideo()
{
	release_vframe(frame1);
	release_vframe(frame2);
	PLUGIN_DESTRUCTOR_MACRO
}

void InterpolateVideo::reset_plugin()
{
	if(frame1)
	{
		release_vframe(frame1);
		frame1 = 0;
		release_vframe(frame2);
		frame2 = 0;
	}
}
PLUGIN_CLASS_METHODS

void InterpolateVideo::fill_border()
{
	if(!frame1->pts_in_frame(range_start_pts))
	{
		frame1->set_pts(range_start_pts);
		frame1 = get_frame(frame1);
	}

	if(!frame2->pts_in_frame(range_end_pts))
	{
		frame2->set_pts(range_end_pts);
		frame2 = get_frame(frame2);
	}
}

VFrame *InterpolateVideo::process_tmpframe(VFrame *frame)
{
	int cmodel = frame->get_color_model();

	if(load_configuration())
		update_gui();

	switch(cmodel)
	{
	case BC_RGBA16161616:
	case BC_AYUV16161616:
		break;
	default:
		unsupported(cmodel);
		return frame;
	}

	if(!frame1)
	{
		frame1 = clone_vframe(frame);
		frame1->set_pts(-1);
	}

	if(!frame2)
	{
		frame2 = clone_vframe(frame);
		frame2->set_pts(-1);
	}

	if(!PTSEQU(range_start_pts, range_end_pts))
	{
// Fill border frames
		fill_border();
// Fraction of lowest frame in output
		double lowest_fraction = 1.0 - ((frame->get_pts() - frame1->get_pts()) /
			(frame2->get_pts() - frame1->get_pts()));

		int fraction0 = lowest_fraction * 0xffff;
		int fraction1 = 0xffff - fraction0;

		CLAMP(fraction0, 0, 0xffff);
		CLAMP(fraction1, 0, 0xffff);

		int wpx = frame->get_w() * 4;
		int h = frame->get_h();

		switch(cmodel)
		{
		case BC_RGBA16161616:
		case BC_AYUV16161616:
			for(int i = 0; i < h; i++)
			{
				uint16_t *in_row0 = (uint16_t*)frame1->get_row_ptr(i);
				uint16_t *in_row1 = (uint16_t*)frame2->get_row_ptr(i);
				uint16_t *out_row = (uint16_t*)frame->get_row_ptr(i);

				for(int j = 0; j < wpx; j++)
				{
					*out_row++ = (*in_row0++ * fraction0 +
						*in_row1++ * fraction1) / 0x10000;
				}
			}
			if(frame1->is_transparent() || frame2->is_transparent())
				frame->set_transparent();
			break;
		}
	}
	return frame;
}

int InterpolateVideo::load_configuration()
{
	KeyFrame *prev_keyframe, *next_keyframe;
	InterpolateVideoConfig old_config;
	double active_input_rate;

	old_config.copy_from(&config);

	if(!(next_keyframe = get_next_keyframe(source_pts)))
		return 0;
	if(!(prev_keyframe = get_prev_keyframe(source_pts)))
		return 0;

// Previous keyframe stays in config object.
	read_data(prev_keyframe);

	ptstime prev_pts = prev_keyframe->pos_time;
	ptstime next_pts = next_keyframe->pos_time;
	if(prev_pts < EPSILON && next_pts < EPSILON)
	{
		next_pts = prev_pts = get_start();
	}

// Get range to average in requested rate
	range_start_pts = prev_pts;
	range_end_pts = next_pts;

// Use keyframes to determine range
	if(config.use_keyframes)
	{
		active_input_rate = get_project_framerate();
// Between keyframe and edge of range or no keyframes
		if(PTSEQU(range_start_pts, range_end_pts))
		{
// Between first keyframe and start of effect
			if(source_pts >= get_start() &&
				source_pts < range_start_pts)
			{
				range_start_pts = get_start();
			}
			else
// Between last keyframe and end of effect
			if(source_pts >= range_start_pts &&
				source_pts < get_end())
			{
// Last frame should be inclusive of current effect
				range_end_pts = get_end() - 1 / active_input_rate;
			}
		}
	}
	else
// Use frame rate
	{
		active_input_rate = config.input_rate;
// Convert to input frame rate
		range_start_pts = floor((source_pts - get_start()) * active_input_rate)
				/ active_input_rate + get_start();
		range_end_pts = source_pts + 1.0 / active_input_rate;
	}
	return !config.equivalent(&old_config);
}

void InterpolateVideo::load_defaults()
{
	defaults = load_defaults_file("interpolatevideo.rc");

	config.input_rate = defaults->get("INPUT_RATE", config.input_rate);
	config.input_rate = Units::fix_framerate(config.input_rate);
	config.use_keyframes = defaults->get("USE_KEYFRAMES", config.use_keyframes);
}

void InterpolateVideo::save_defaults()
{
	defaults->update("INPUT_RATE", config.input_rate);
	defaults->update("USE_KEYFRAMES", config.use_keyframes);
	defaults->save();
}

void InterpolateVideo::save_data(KeyFrame *keyframe)
{
	FileXML output;

	output.tag.set_title("INTERPOLATEVIDEO");
	output.tag.set_property("INPUT_RATE", config.input_rate);
	output.tag.set_property("USE_KEYFRAMES", config.use_keyframes);
	output.append_tag();
	output.tag.set_title("/INTERPOLATEVIDEO");
	output.append_tag();
	keyframe->set_data(output.string);
}

void InterpolateVideo::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());

	while(!input.read_tag())
	{
		if(input.tag.title_is("INTERPOLATEVIDEO"))
		{
			config.input_rate = input.tag.get_property("INPUT_RATE", config.input_rate);
			config.input_rate = Units::fix_framerate(config.input_rate);
			config.use_keyframes = input.tag.get_property("USE_KEYFRAMES", config.use_keyframes);
		}
	}
}
