// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

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
	overlayer = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

TranslateMain::~TranslateMain()
{
	delete overlayer;
	PLUGIN_DESTRUCTOR_MACRO
}

void TranslateMain::reset_plugin()
{
	if(overlayer)
	{
		delete overlayer;
		overlayer = 0;
	}
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
	keyframe->set_data(output.string);
}

void TranslateMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());

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

VFrame *TranslateMain::process_tmpframe(VFrame *input)
{
	VFrame *output;
	int cmodel = input->get_color_model();

	switch(cmodel)
	{
	case BC_RGBA16161616:
	case BC_AYUV16161616:
		break;
	default:
		unsupported(cmodel);
		return input;
	}

	if(load_configuration())
		update_gui();

	if(!overlayer)
		overlayer = new OverlayFrame(get_project_smp());

	output = clone_vframe(input);
	output->clear_frame();

	overlayer->overlay(output,
		input,
		config.in_x,
		config.in_y,
		config.in_x + config.in_w,
		config.in_y + config.in_h,
		config.out_x,
		config.out_y,
		config.out_x + config.out_w,
		config.out_y + config.out_h,
		1,
		TRANSFER_REPLACE,
		get_interpolation_type());
	release_vframe(input);
	output->set_transparent();
	return output;
}
