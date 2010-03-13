
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
#include "language.h"
#include "picon_png.h"
#include "scale.h"
#include "scalewin.h"

#include <string.h>


REGISTER_PLUGIN(ScaleMain)



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
	int64_t prev_frame, 
	int64_t next_frame, 
	int64_t current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);

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
	PLUGIN_DESTRUCTOR_MACRO

	if(overlayer) delete overlayer;
	overlayer = 0;
}

const char* ScaleMain::plugin_title() { return N_("Scale"); }
int ScaleMain::is_realtime() { return 1; }

NEW_PICON_MACRO(ScaleMain)

int ScaleMain::load_defaults()
{
	char directory[1024], string[1024];
// set the default directory
	sprintf(directory, "%sscale.rc", BCASTDIR);

// load the defaults
	defaults = new BC_Hash(directory);
	defaults->load();

	config.w = defaults->get("WIDTH", config.w);
	config.h = defaults->get("HEIGHT", config.h);
	config.constrain = defaults->get("CONSTRAIN", config.constrain);
//printf("ScaleMain::load_defaults %f %f\n",config.w  , config.h);
}

int ScaleMain::save_defaults()
{
	defaults->update("WIDTH", config.w);
	defaults->update("HEIGHT", config.h);
	defaults->update("CONSTRAIN", config.constrain);
	defaults->save();
}

LOAD_CONFIGURATION_MACRO(ScaleMain, ScaleConfig)


void ScaleMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_string(keyframe->data, MESSAGESIZE);

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
	output.terminate_string();
// data is now in *text
}

void ScaleMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->data, strlen(keyframe->data));

	int result = 0;
	config.constrain = 0;

	while(!result)
	{
		result = input.read_tag();

		if(!result)
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
}








int ScaleMain::process_buffer(VFrame *frame,
	int64_t start_position,
	double frame_rate)
{
	VFrame *input, *output;
	
	input = frame;
	output = frame;

	load_configuration();

	read_frame(frame, 
		0, 
		start_position, 
		frame_rate,
		get_use_opengl());

// No scaling
	if(config.w == 1 && config.h == 1)
		return 0;

	if(get_use_opengl()) return run_opengl();

	VFrame *temp_frame = new_temp(frame->get_w(), 
			frame->get_h(),
			frame->get_color_model());
	temp_frame->copy_from(frame);
	input = temp_frame;

	if(!overlayer)
	{
		overlayer = new OverlayFrame(smp + 1);
	}


// Perform scaling
	float in_x1, in_x2, in_y1, in_y2, out_x1, out_x2, out_y1, out_y2;
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

// printf("ScaleMain::process_realtime 3 output=%p input=%p config.w=%f config.h=%f"
// 	"%f %f %f %f -> %f %f %f %f\n", 
// 	output,
// 	input,
// 	config.w, 
// 	config.h,
// 	in_x1, 
// 	in_y1, 
// 	in_x2, 
// 	in_y2,
// 	out_x1, 
// 	out_y1, 
// 	out_x2, 
// 	out_y2);
	overlayer->overlay(output, 
		input,
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

	return 0;
}

void ScaleMain::calculate_transfer(VFrame *frame,
	float &in_x1, 
	float &in_x2, 
	float &in_y1, 
	float &in_y2, 
	float &out_x1, 
	float &out_x2, 
	float &out_y1, 
	float &out_y2)
{
	float center_x, center_y;
	center_x = (float)frame->get_w() / 2;
	center_y = (float)frame->get_h() / 2;
	in_x1 = 0;
	in_x2 = frame->get_w();
	in_y1 = 0;
	in_y2 = frame->get_h();
	out_x1 = (float)center_x - (float)frame->get_w() * config.w / 2;
	out_x2 = (float)center_x + (float)frame->get_w() * config.w / 2;
	out_y1 = (float)center_y - (float)frame->get_h() * config.h / 2;
	out_y2 = (float)center_y + (float)frame->get_h() * config.h / 2;



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

int ScaleMain::handle_opengl()
{
#ifdef HAVE_GL
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
#endif
}



SHOW_GUI_MACRO(ScaleMain, ScaleThread)
RAISE_WINDOW_MACRO(ScaleMain)
SET_STRING_MACRO(ScaleMain)

void ScaleMain::update_gui()
{
	if(thread) 
	{
		load_configuration();
		thread->window->lock_window();
		thread->window->width->update(config.w);
		thread->window->height->update(config.h);
		thread->window->constrain->update(config.constrain);
		thread->window->unlock_window();
	}
}



