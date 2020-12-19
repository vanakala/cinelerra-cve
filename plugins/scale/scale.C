// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "clip.h"
#include "filexml.h"
#include "language.h"
#include "picon_png.h"
#include "scale.h"

#include <string.h>


REGISTER_PLUGIN


ScaleConfig::ScaleConfig()
{
	w = 1;
	h = 1;
	constrain = 0;
}

void ScaleConfig::copy_from(ScaleConfig &src)
{
	w = src.w;
	h = src.h;
	constrain = src.constrain;
}

int ScaleConfig::equivalent(ScaleConfig &src)
{
	return EQUIV(w, src.w) && 
		EQUIV(h, src.h) && 
		constrain == src.constrain;
}

void ScaleConfig::interpolate(ScaleConfig &prev, 
	ScaleConfig &next, 
	ptstime prev_pts,
	ptstime next_pts,
	ptstime current_pts)
{
	PLUGIN_CONFIG_INTERPOLATE_MACRO

	this->w = prev.w * prev_scale + next.w * next_scale;
	this->h = prev.h * prev_scale + next.h * next_scale;
	this->constrain = prev.constrain;
}


ScaleMain::ScaleMain(PluginServer *server)
 : PluginVClient(server)
{
	overlayer = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

ScaleMain::~ScaleMain()
{
	delete overlayer;
	PLUGIN_DESTRUCTOR_MACRO
}

void ScaleMain::reset_plugin()
{
	if(overlayer)
	{
		delete overlayer;
		overlayer = 0;
	}
}

PLUGIN_CLASS_METHODS

void ScaleMain::load_defaults()
{
	defaults = load_defaults_file("scale.rc");

	config.w = defaults->get("WIDTH", config.w);
	config.h = defaults->get("HEIGHT", config.h);
	config.constrain = defaults->get("CONSTRAIN", config.constrain);
}

void ScaleMain::save_defaults()
{
	defaults->update("WIDTH", config.w);
	defaults->update("HEIGHT", config.h);
	defaults->update("CONSTRAIN", config.constrain);
	defaults->save();
}

void ScaleMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

// Store data
	output.tag.set_title("SCALE");
	output.tag.set_property("WIDTH", config.w);
	output.tag.set_property("HEIGHT", config.h);
	output.append_tag();

	if(config.constrain)
	{
		output.tag.set_title("CONSTRAIN");
		output.append_tag();
		output.tag.set_title("/CONSTRAIN");
		output.append_tag();
	}
	output.tag.set_title("/SCALE");
	output.append_tag();
	keyframe->set_data(output.string);
}

void ScaleMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());

	int result = 0;
	config.constrain = 0;

	while(!input.read_tag())
	{
		if(input.tag.title_is("SCALE"))
		{
			config.w = input.tag.get_property("WIDTH", config.w);
			config.h = input.tag.get_property("HEIGHT", config.h);
		}
		else
		if(input.tag.title_is("CONSTRAIN"))
		{
			config.constrain = 1;
		}
	}
}

VFrame *ScaleMain::process_tmpframe(VFrame *frame)
{
	VFrame *output;
	int cmodel = frame->get_color_model();

	switch(cmodel)
	{
	case BC_RGBA16161616:
	case BC_AYUV16161616:
		break;
	default:
		unsupported(cmodel);
		return frame;
	}

	if(load_configuration())
		update_gui();

// No scaling
	if(EQUIV(config.w, 1) && EQUIV(config.h,1))
		return frame;

	output = clone_vframe(frame);

	if(!overlayer)
		overlayer = new OverlayFrame(get_project_smp());

// Perform scaling
	double in_x1, in_x2, in_y1, in_y2, out_x1, out_x2, out_y1, out_y2;

	calculate_transfer(output,
		in_x1, 
		in_x2, 
		in_y1, 
		in_y2, 
		out_x1, 
		out_x2, 
		out_y1, 
		out_y2);
	output->clear_frame();

	overlayer->overlay(output, 
		frame,
		in_x1, 
		in_y1, 
		in_x2, 
		in_y2,
		out_x1, 
		out_y1, 
		out_x2, 
		out_y2, 
		1,
		TRANSFER_REPLACE,
		get_interpolation_type());

	release_vframe(frame);
	output->set_transparent();
	return output;
}

void ScaleMain::calculate_transfer(VFrame *frame,
	double &in_x1, double &in_x2,
	double &in_y1, double &in_y2,
	double &out_x1, double &out_x2,
	double &out_y1, double &out_y2)
{
	double center_x, center_y;

	center_x = (double)frame->get_w() / 2;
	center_y = (double)frame->get_h() / 2;

	in_x1 = 0;
	in_x2 = frame->get_w();
	in_y1 = 0;
	in_y2 = frame->get_h();

	out_x1 = center_x - frame->get_w() * config.w / 2;
	out_x2 = center_x + frame->get_w() * config.w / 2;
	out_y1 = center_y - frame->get_h() * config.h / 2;
	out_y2 = center_y + frame->get_h() * config.h / 2;

	if(out_x1 < 0)
	{
		in_x1 += -out_x1 / config.w;
		out_x1 = 0;
	}

	if(out_x2 > frame->get_w())
	{
		in_x2 -= (out_x2 - frame->get_w()) / config.w;
		out_x2 = frame->get_w();
	}

	if(out_y1 < 0)
	{
		in_y1 += -out_y1 / config.h;
		out_y1 = 0;
	}

	if(out_y2 > frame->get_h())
	{
		in_y2 -= (out_y2 - frame->get_h()) / config.h;
		out_y2 = frame->get_h();
	}
}

void ScaleMain::handle_opengl()
{
#ifdef HAVE_GL
/* FIXIT
	float in_x1, in_x2, in_y1, in_y2, out_x1, out_x2, out_y1, out_y2;
	calculate_transfer(get_output(),
		in_x1, 
		in_x2, 
		in_y1, 
		in_y2, 
		out_x1, 
		out_x2, 
		out_y1, 
		out_y2);
	get_output()->to_texture();
	get_output()->enable_opengl();
	get_output()->init_screen();
	get_output()->clear_pbuffer();
	get_output()->bind_texture(0);
	get_output()->draw_texture(in_x1, 
		in_y1, 
		in_x2, 
		in_y2, 
		out_x1, 
		out_y1, 
		out_x2, 
		out_y2);
	get_output()->set_opengl_state(VFrame::SCREEN);
	*/
#endif
}
