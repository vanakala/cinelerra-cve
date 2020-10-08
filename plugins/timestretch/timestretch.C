// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "aframe.h"
#include "avresample.h"
#include "clip.h"
#include "bchash.h"
#include "bctitle.h"
#include "picon_png.h"
#include "timestretch.h"
#include "filexml.h"

#include <string.h>

REGISTER_PLUGIN


TimeStretch::TimeStretch(PluginServer *server)
 : PluginAClient(server)
{
	PLUGIN_CONSTRUCTOR_MACRO
	pitch = 0;
	resample = 0;
	input_pts = -1;
	prev_input = -1;
	prev_frame = -1;
}

TimeStretch::~TimeStretch()
{
	PLUGIN_DESTRUCTOR_MACRO
	delete pitch;
	delete resample;
}

void TimeStretch::reset_plugin()
{
	if(pitch)
	{
		delete pitch;
		pitch = 0;
		delete resample;
		resample = 0;
	}
}

PLUGIN_CLASS_METHODS

void TimeStretch::read_data(KeyFrame *keyframe)
{
	FileXML input;
	int result = 0;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());

	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("TIMESTRETCH"))
			{
				config.scale = input.tag.get_property("SCALE", config.scale);
			}
		}
	}
}

void TimeStretch::save_data(KeyFrame *keyframe)
{
	FileXML output;

	output.tag.set_title("TIMESTRETCH");
	output.tag.set_property("SCALE", config.scale);
	output.append_tag();
	output.tag.set_title("/TIMESTRETCH");
	output.append_tag();
	keyframe->set_data(output.string);
}

void TimeStretch::load_defaults()
{
// load the defaults
	defaults = load_defaults_file("timestretch.rc");

	config.scale = defaults->get("SCALE", (double)1);
}

void TimeStretch::save_defaults()
{
	defaults->update("SCALE", config.scale);
	defaults->save();
}


TimeStretchConfig::TimeStretchConfig()
{
	scale = 1.0;
}

int TimeStretchConfig::equivalent(TimeStretchConfig &that)
{
	return EQUIV(scale, that.scale);
}

void TimeStretchConfig::copy_from(TimeStretchConfig &that)
{
	scale = that.scale;
}

int TimeStretch::load_configuration()
{
	return need_reconfigure;
}

AFrame *TimeStretch::process_tmpframe(AFrame *aframe)
{
	int output_pos = 0;
	int fragment_size;
	AFrame *src_frame, *tmp_frame;
	int do_reconfigure = load_configuration();

	input_frame = aframe;
	src_frame = aframe;

	if(!pitch)
	{
		pitch = new Pitch(aframe->get_samplerate(),
			aframe->get_buffer_length() / 4);
		resample = new AVResample();
		do_reconfigure = 1;
	}
	calculate_pts();
	input_pts = aframe->round_to_sample(input_pts);

	if(do_reconfigure || input_pts < prev_frame || input_pts > prev_input + EPSILON)
	{
		pitch->set_scale(config.scale);
		resample->reset();
		do_reconfigure = 0;
		prev_input = -1;
		update_gui();
	}

	if(!PTSEQU(aframe->get_pts(), input_pts))
		src_frame = 0;
	tmp_frame = 0;

	while(output_pos < aframe->get_length())
	{
		if(src_frame)
		{
			if(resample->resample_frame(src_frame, config.scale))
			{
				abort_plugin(_("Reasmpling failed"));
				release_aframe(tmp_frame);
				return aframe;
			}
			prev_frame = src_frame->get_pts();
			prev_input = src_frame->get_end_pts();
		}
		fragment_size = resample->get_output_size();

		if(fragment_size > 0)
		{
			if(fragment_size > aframe->get_length() - output_pos)
				fragment_size = aframe->get_length() - output_pos;
			resample->read_output(aframe->buffer + output_pos,
				fragment_size);
			output_pos += fragment_size;
		}

		if(prev_input < 0)
			prev_input = input_pts;

		if(output_pos < aframe->get_length())
		{
			if(!tmp_frame)
				tmp_frame = clone_aframe(aframe);
			tmp_frame->set_fill_request(prev_input,
				tmp_frame->get_buffer_length());
			tmp_frame = get_frame(tmp_frame);
			src_frame = tmp_frame;
		}
	}
	release_aframe(tmp_frame);
	aframe = pitch->process_frame(aframe);
	return aframe;
}

void TimeStretch::calculate_pts()
{
	KeyFrame *keyframe;
	ptstime pts;

	keyframe = get_first_keyframe();
	input_pts = pts = get_start();

	if(keyframe)
	{
		read_data(keyframe);
		keyframe = (KeyFrame*)keyframe->next;

		while(keyframe && keyframe->pos_time < source_pts)
		{
			input_pts += (keyframe->pos_time - pts) / config.scale;
			pts = keyframe->pos_time;
			read_data(keyframe);
			keyframe = (KeyFrame*)keyframe->next;
		}
	}
	input_pts += (source_pts - pts) / config.scale;
}

PLUGIN_THREAD_METHODS

TimeStretchWindow::TimeStretchWindow(TimeStretch *plugin, int x, int y)
 : PluginWindow(plugin->gui_string, 
	x,
	y, 
	150, 
	50)
{
	x = y = 10;

	add_subwindow(new BC_Title(x, y, _("Scale:")));
	x += 70;
	add_subwindow(scale = new TimeStretchScale(plugin, x, y));
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

void TimeStretchWindow::update()
{
	scale->update(plugin->config.scale);
}


TimeStretchScale::TimeStretchScale(TimeStretch *plugin, int x, int y)
 : BC_FPot(x, y, (float)plugin->config.scale, .3, 2)
{
	this->plugin = plugin;
	set_precision(0.001);
}

int TimeStretchScale::handle_event()
{
	plugin->config.scale = get_value();
	plugin->send_configure_change();
	return 1;
}
