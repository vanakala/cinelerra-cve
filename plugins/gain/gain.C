// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "aframe.h"
#include "clip.h"
#include "bchash.h"
#include "filexml.h"
#include "picon_png.h"
#include "gain.h"
#include "gainwindow.h"


REGISTER_PLUGIN


GainConfig::GainConfig()
{
	level = 0.0;
	levelslope = 0.0;
}

int GainConfig::equivalent(GainConfig &that)
{
	return EQUIV(level, that.level);
}

void GainConfig::copy_from(GainConfig &that)
{
	this->level = that.level;
}

void GainConfig::interpolate(GainConfig &prev, 
	GainConfig &next, 
	ptstime prev_pts,
	ptstime next_pts,
	ptstime current_pts)
{
	PLUGIN_CONFIG_INTERPOLATE_MACRO
	level = prev.level * prev_scale + next.level * next_scale;
	levelslope = (next.level - prev.level) / (next_pts - prev_pts);
}


Gain::Gain(PluginServer *server)
 : PluginAClient(server)
{
	PLUGIN_CONSTRUCTOR_MACRO
}

Gain::~Gain()
{
	PLUGIN_DESTRUCTOR_MACRO
}

PLUGIN_CLASS_METHODS

AFrame *Gain::process_tmpframe(AFrame *input)
{
	int size = input->get_length();
	double *ipp = input->buffer;
	double slope, gain;
	int begin_slope, end_slope;

	if(load_configuration())
		update_gui();

	if(fabs(config.levelslope) > EPSILON)
	{
		slope = config.levelslope / input->get_samplerate();
		begin_slope = 0;
		end_slope = size;
		if(config.slope_start > input->get_pts())
			begin_slope = input->to_samples(config.slope_start - input->get_pts());
		if(config.slope_end < input->get_pts() + input->get_duration())
			end_slope = input->to_samples(config.slope_end - input->get_pts());
	}
	else
	{
		gain = db.fromdb(config.level);
		slope = 0;
	}

	for(int i = 0; i < size; i++)
	{
		if(fabs(slope) > EPSILON)
		{
			if(i < begin_slope)
				gain = db.fromdb(config.level);
			else if(i > end_slope)
				gain = db.fromdb(config.level + slope * end_slope);
			else
				gain = db.fromdb(config.level + slope * i);
		}
		*ipp++ *= gain;
	}
	return input;
}

void Gain::load_defaults()
{
	defaults = load_defaults_file("gain.rc");
	config.level = defaults->get("LEVEL", config.level);
	config.levelslope = 0;
}

void Gain::save_defaults()
{
	defaults->update("LEVEL", config.level);
	defaults->save();
}

void Gain::save_data(KeyFrame *keyframe)
{
	FileXML output;

	output.tag.set_title("GAIN");
	output.tag.set_property("LEVEL", config.level);
	output.append_tag();
	output.tag.set_title("/GAIN");
	output.append_tag();
	keyframe->set_data(output.string);
}

void Gain::read_data(KeyFrame *keyframe)
{
	FileXML input;
	int result = 0;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());

	result = input.read_tag();

	if(!result)
	{
		if(input.tag.title_is("GAIN"))
		{
			config.level = input.tag.get_property("LEVEL", config.level);
		}
	}
	config.levelslope = 0;
}
