
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
#include "confirmsave.h"
#include "bchash.h"
#include "errorbox.h"
#include "filexml.h"
#include "language.h"
#include "picon_png.h"
#include "gain.h"
#include "gainwindow.h"

#include "vframe.h"

#include <string.h>


REGISTER_PLUGIN(Gain)


GainConfig::GainConfig()
{
	level = 0.0;
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
	double next_scale = (current_pts - prev_pts) / (next_pts - prev_pts);
	double prev_scale = (next_pts - current_pts) / (next_pts - prev_pts);
	level = prev.level * prev_scale + next.level * next_scale;
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

const char* Gain::plugin_title() { return N_("Gain"); }
int Gain::is_realtime() { return 1; }
int Gain::has_pts_api() { return 1; }

SHOW_GUI_MACRO(Gain, GainThread)
SET_STRING_MACRO(Gain)
RAISE_WINDOW_MACRO(Gain)
NEW_PICON_MACRO(Gain)
LOAD_PTS_CONFIGURATION_MACRO(Gain, GainConfig)

void Gain::process_frame_realtime(AFrame *input, AFrame *output)
{
	int size = input->length;
	double *ipp = input->buffer;
	double *opp = output->buffer;

	load_configuration();

	double gain = db.fromdb(config.level);

	if(input != output)
		output->copy_of(input);

	for(int i = 0; i < size; i++)
	{
		*opp++ = *ipp++ * gain;
	}
}

void Gain::load_defaults()
{
	char directory[BCTEXTLEN];

// set the default directory
	sprintf(directory, "%sgain.rc", get_defaultdir());

// load the defaults

	defaults = new BC_Hash(directory);

	defaults->load();

	config.level = defaults->get("LEVEL", config.level);
}

void Gain::save_defaults()
{
	defaults->update("LEVEL", config.level);
	defaults->save();
}

void Gain::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause xml file to store data directly in text
	output.set_shared_string(keyframe->data, MESSAGESIZE);

	output.tag.set_title("GAIN");
	output.tag.set_property("LEVEL", config.level);
	output.append_tag();
	output.tag.set_title("/GAIN");
	output.append_tag();
	output.append_newline();
	output.terminate_string();
}

void Gain::read_data(KeyFrame *keyframe)
{
	FileXML input;
// cause xml file to read directly from text
	input.set_shared_string(keyframe->data, strlen(keyframe->data));
	int result = 0;

	result = input.read_tag();

	if(!result)
	{
		if(input.tag.title_is("GAIN"))
		{
			config.level = input.tag.get_property("LEVEL", config.level);
		}
	}
}

void Gain::update_gui()
{
	if(thread)
	{
		load_configuration();
		thread->window->lock_window();
		thread->window->level->update(config.level);
		thread->window->unlock_window();
	}
}
