
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
#include "filexml.h"
#include "picon_png.h"
#include "translate.h"
#include "translatewin.h"

#include <string.h>

REGISTER_PLUGIN

TranslateConfig::TranslateConfig()
{
	in_x = 0;
	in_y = 0;
	in_w = 720;
	in_h = 480;
	out_x = 0;
	out_y = 0;
	out_w = 720;
	out_h = 480;
}

int TranslateConfig::equivalent(TranslateConfig &that)
{
	return EQUIV(in_x, that.in_x) && 
		EQUIV(in_y, that.in_y) && 
		EQUIV(in_w, that.in_w) && 
		EQUIV(in_h, that.in_h) &&
		EQUIV(out_x, that.out_x) && 
		EQUIV(out_y, that.out_y) && 
		EQUIV(out_w, that.out_w) &&
		EQUIV(out_h, that.out_h);
}

void TranslateConfig::copy_from(TranslateConfig &that)
{
	in_x = that.in_x;
	in_y = that.in_y;
	in_w = that.in_w;
	in_h = that.in_h;
	out_x = that.out_x;
	out_y = that.out_y;
	out_w = that.out_w;
	out_h = that.out_h;
}

void TranslateConfig::interpolate(TranslateConfig &prev, 
	TranslateConfig &next, 
	ptstime prev_pts,
	ptstime next_pts,
	ptstime current_pts)
{
	PLUGIN_CONFIG_INTERPOLATE_MACRO

	this->in_x = prev.in_x * prev_scale + next.in_x * next_scale;
	this->in_y = prev.in_y * prev_scale + next.in_y * next_scale;
	this->in_w = prev.in_w * prev_scale + next.in_w * next_scale;
	this->in_h = prev.in_h * prev_scale + next.in_h * next_scale;
	this->out_x = prev.out_x * prev_scale + next.out_x * next_scale;
	this->out_y = prev.out_y * prev_scale + next.out_y * next_scale;
	this->out_w = prev.out_w * prev_scale + next.out_w * next_scale;
	this->out_h = prev.out_h * prev_scale + next.out_h * next_scale;
}


TranslateMain::TranslateMain(PluginServer *server)
 : PluginVClient(server)
{
	temp_frame = 0;
	overlayer = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

TranslateMain::~TranslateMain()
{
	if(temp_frame) delete temp_frame;
	temp_frame = 0;
	if(overlayer) delete overlayer;
	overlayer = 0;
	PLUGIN_DESTRUCTOR_MACRO
}

PLUGIN_CLASS_METHODS

void TranslateMain::load_defaults()
{
// load the defaults
	defaults = load_defaults_file("translate.rc");

	config.in_x = defaults->get("IN_X", config.in_x);
	config.in_y = defaults->get("IN_Y", config.in_y);
	config.in_w = defaults->get("IN_W", config.in_w);
	config.in_h = defaults->get("IN_H", config.in_h);
	config.out_x = defaults->get("OUT_X", config.out_x);
	config.out_y = defaults->get("OUT_Y", config.out_y);
	config.out_w = defaults->get("OUT_W", config.out_w);
	config.out_h = defaults->get("OUT_H", config.out_h);
}

void TranslateMain::save_defaults()
{
	defaults->update("IN_X", config.in_x);
	defaults->update("IN_Y", config.in_y);
	defaults->update("IN_W", config.in_w);
	defaults->update("IN_H", config.in_h);
	defaults->update("OUT_X", config.out_x);
	defaults->update("OUT_Y", config.out_y);
	defaults->update("OUT_W", config.out_w);
	defaults->update("OUT_H", config.out_h);
	defaults->save();
}

void TranslateMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_string(keyframe->data, MESSAGESIZE);

// Store data
	output.tag.set_title("TRANSLATE");
	output.tag.set_property("IN_X", config.in_x);
	output.tag.set_property("IN_Y", config.in_y);
	output.tag.set_property("IN_W", config.in_w);
	output.tag.set_property("IN_H", config.in_h);
	output.tag.set_property("OUT_X", config.out_x);
	output.tag.set_property("OUT_Y", config.out_y);
	output.tag.set_property("OUT_W", config.out_w);
	output.tag.set_property("OUT_H", config.out_h);
	output.append_tag();
	output.tag.set_title("/TRANSLATE");
	output.append_tag();

	output.terminate_string();
// data is now in *text
}

void TranslateMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->data, strlen(keyframe->data));

	int result = 0;

	while(!input.read_tag())
	{
		if(input.tag.title_is("TRANSLATE"))
		{
			config.in_x = input.tag.get_property("IN_X", config.in_x);
			config.in_y = input.tag.get_property("IN_Y", config.in_y);
			config.in_w = input.tag.get_property("IN_W", config.in_w);
			config.in_h = input.tag.get_property("IN_H", config.in_h);
			config.out_x = input.tag.get_property("OUT_X", config.out_x);
			config.out_y = input.tag.get_property("OUT_Y", config.out_y);
			config.out_w = input.tag.get_property("OUT_W", config.out_w);
			config.out_h = input.tag.get_property("OUT_H", config.out_h);
		}
	}
}

void TranslateMain::process_realtime(VFrame *input_ptr, VFrame *output_ptr)
{
	VFrame *input, *output;
	float ix1, ix2, iy1, iy2;
	float ox1, ox2, oy1, oy2;
	float cx, cy;

	input = input_ptr;
	output = output_ptr;

	load_configuration();

	if(input->get_rows()[0] == output->get_rows()[0])
	{
		if(!temp_frame) 
			temp_frame = new VFrame(0, 
				input_ptr->get_w(), 
				input_ptr->get_h(),
				input->get_color_model());
		temp_frame->copy_from(input);
		input = temp_frame;
	}

	if(!overlayer)
	{
		overlayer = new OverlayFrame(smp + 1);
	}

	if(config.in_x < 0 || config.in_x > output->get_w())
		config.in_x = 0;
	if(config.in_y < 0 || config.in_y > output->get_h())
		config.in_y = 0;
	if(config.in_w < 0 || config.in_w > output->get_w())
		config.in_w = output->get_w();
	if(config.in_h < 0 || config.in_h > output->get_h())
		config.in_h = output->get_h();

	if(config.in_w > 0 && config.in_h > 0)
	{
		output->clear_frame();
		cx = config.out_w / config.in_w;
		cy = config.out_h / config.in_h;
		ix1 = config.in_x;
		iy1 = config.in_y;
		ix2 = config.in_x + config.in_w;
		iy2 = config.in_y + config.in_h;
		ox1 = config.out_x;
		oy1 = config.out_y;
		ox2 = config.out_x + config.out_w;
		oy2 = config.out_y + config.out_h;
		if(ox1 < 0)
		{
			ix1 += -ox1 / cx;
			ox1 = 0;
		}
		if(oy1 < 0)
		{
			iy1 += -oy1 / cy;
			oy1 = 0;
		}
		if(ox2 > output->get_w())
		{
			ix2 -= (ox2 - output->get_w()) / cx;
			ox2 = output->get_w();
		}
		if(oy2 > output->get_h())
		{
			iy2 -= (oy2 - output->get_h()) / cy;
			oy2 = output->get_h();
		}
		overlayer->overlay(output,
			input,
			ix1,
			iy1,
			ix2,
			iy2,
			ox1,
			oy1,
			ox2,
			oy2,
			1,
			TRANSFER_REPLACE,
			get_interpolation_type());
	}
}
