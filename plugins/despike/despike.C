
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

#include "clip.h"
#include "bchash.h"
#include "filexml.h"
#include "picon_png.h"
#include "despike.h"
#include "despikewindow.h"

#include "vframe.h"

#include <string.h>

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

void Despike::process_realtime(AFrame *input, AFrame *output)
{
	int size = input->length;
	double *ipp = input->buffer;
	double *opp;

	load_configuration();

	double threshold = db.fromdb(config.level);
	double change = db.fromdb(config.slope);

	if(input != output)
		output->copy_of(input);

	opp = output->buffer;

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
			*opp++ = *ipp;
			last_sample = *ipp++;
		}
	}
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

// cause xml file to store data directly in text
	output.set_shared_string(keyframe->data, MESSAGESIZE);

	output.tag.set_title("DESPIKE");
	output.tag.set_property("LEVEL", config.level);
	output.tag.set_property("SLOPE", config.slope);
	output.append_tag();
	output.tag.set_title("/DESPIKE");
	output.append_tag();
	output.append_newline();
	output.terminate_string();
}

void Despike::read_data(KeyFrame *keyframe)
{
	FileXML input;
// cause xml file to read directly from text
	input.set_shared_string(keyframe->data, strlen(keyframe->data));
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
