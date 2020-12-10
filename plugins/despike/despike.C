// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "aframe.h"
#include "clip.h"
#include "bchash.h"
#include "filexml.h"
#include "picon_png.h"
#include "despike.h"
#include "despikewindow.h"

REGISTER_PLUGIN

Despike::Despike(PluginServer *server)
 : PluginAClient(server)
{
	last_sample = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

Despike::~Despike()
{
	PLUGIN_DESTRUCTOR_MACRO
}

PLUGIN_CLASS_METHODS

AFrame *Despike::process_tmpframe(AFrame *input)
{
	int size = input->get_length();
	double *ipp = input->buffer;
	double *opp;

	if(load_configuration())
		update_gui();

	double threshold = db.fromdb(config.level);
	double change = db.fromdb(config.slope);

	opp = input->buffer;

	for(int i = 0; i < size; i++)
	{
		if(fabs(*ipp) > threshold ||
			fabs(*ipp) - fabs(last_sample) > change)
		{
			*opp++ = last_sample;
			ipp++;
		}
		else
		{
			last_sample = *ipp++;
		}
	}
	return input;
}

void Despike::load_defaults()
{
	defaults = load_defaults_file("despike.rc");

	config.level = defaults->get("LEVEL", (double)0);
	config.slope = defaults->get("SLOPE", (double)0);
}

void Despike::save_defaults()
{
	defaults->update("LEVEL", config.level);
	defaults->update("SLOPE", config.slope);
	defaults->save();
}

void Despike::save_data(KeyFrame *keyframe)
{
	FileXML output;

	output.tag.set_title("DESPIKE");
	output.tag.set_property("LEVEL", config.level);
	output.tag.set_property("SLOPE", config.slope);
	output.append_tag();
	output.tag.set_title("/DESPIKE");
	output.append_tag();
	output.append_newline();
	keyframe->set_data(output.string);
}

void Despike::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());
	int result = 0;

	result = input.read_tag();

	if(!result)
	{
		if(input.tag.title_is("DESPIKE"))
		{
			config.level = input.tag.get_property("LEVEL", config.level);
			config.slope = input.tag.get_property("SLOPE", config.slope);
		}
	}
}


DespikeConfig::DespikeConfig()
{
	level = 0;
	slope = 0;
}

int DespikeConfig::equivalent(DespikeConfig &that)
{
	return EQUIV(level, that.level) && 
		EQUIV(slope, that.slope);
}

void DespikeConfig::copy_from(DespikeConfig &that)
{
	level = that.level;
	slope = that.slope;
}

void DespikeConfig::interpolate(DespikeConfig &prev, 
	DespikeConfig &next, 
	ptstime prev_pts,
	ptstime next_pts,
	ptstime current_pts)
{
	PLUGIN_CONFIG_INTERPOLATE_MACRO

	this->level = prev.level * prev_scale + next.level * next_scale;
	this->slope = prev.slope * prev_scale + next.slope * next_scale;
}
