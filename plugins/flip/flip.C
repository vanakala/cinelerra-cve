
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
#include "colormodels.inc"
#include "bchash.h"
#include "filexml.h"
#include "flip.h"
#include "flipwindow.h"
#include "language.h"
#include "picon_png.h"

#include <stdint.h>
#include <string.h>

REGISTER_PLUGIN


FlipConfig::FlipConfig()
{
	flip_horizontal = 0;
	flip_vertical = 0;
}

void FlipConfig::copy_from(FlipConfig &that)
{
	flip_horizontal = that.flip_horizontal;
	flip_vertical = that.flip_vertical;
}

int FlipConfig::equivalent(FlipConfig &that)
{
	return flip_horizontal == that.flip_horizontal &&
		flip_vertical == that.flip_vertical;
}

void FlipConfig::interpolate(FlipConfig &prev, 
	FlipConfig &next, 
	ptstime prev_pts,
	ptstime next_pts,
	ptstime current_pts)
{
	this->flip_horizontal = prev.flip_horizontal;
	this->flip_vertical = prev.flip_vertical;
}


FlipMain::FlipMain(PluginServer *server)
 : PluginVClient(server)
{
	PLUGIN_CONSTRUCTOR_MACRO
}

FlipMain::~FlipMain()
{
	PLUGIN_DESTRUCTOR_MACRO
}

PLUGIN_CLASS_METHODS

#define SWAP_PIXELS(type, components, in, out) \
{ \
	type temp = in[0]; \
	in[0] = out[0]; \
	out[0] = temp; \
 \
	temp = in[1]; \
	in[1] = out[1]; \
	out[1] = temp; \
 \
	temp = in[2]; \
	in[2] = out[2]; \
	out[2] = temp; \
 \
	if(components == 4) \
	{ \
		temp = in[3]; \
		in[3] = out[3]; \
		out[3] = temp; \
	} \
}

#define FLIP_MACRO(type, components) \
{ \
	type *input_row, *output_row; \
 \
	if(config.flip_vertical) \
	{ \
		for(i = 0, j = h - 1; i < h / 2; i++, j--) \
		{ \
			input_row = (type*)frame->get_row_ptr(i); \
			output_row = (type*)frame->get_row_ptr(j); \
			for(k = 0; k < w; k++) \
			{ \
				SWAP_PIXELS(type, components, output_row, input_row); \
				output_row += components; \
				input_row += components; \
			} \
		} \
	} \
 \
	if(config.flip_horizontal) \
	{ \
		for(i = 0; i < h; i++) \
		{ \
			input_row = (type*)frame->get_row_ptr(i); \
			output_row = (type*)frame->get_row_ptr(i) + (w - 1) * components; \
			for(k = 0; k < w / 2; k++) \
			{ \
				SWAP_PIXELS(type, components, output_row, input_row); \
				input_row += components; \
				output_row -= components; \
			} \
		} \
	} \
}

void FlipMain::process_frame(VFrame *frame)
{
	int i, j, k, l;
	int w = frame->get_w();
	int h = frame->get_h();
	int colormodel = frame->get_color_model();

	load_configuration();

	get_frame(frame, get_use_opengl());

	if(get_use_opengl()) 
	{
		if(config.flip_vertical || config.flip_horizontal)
			run_opengl();
	}

	switch(colormodel)
	{
	case BC_RGB888:
	case BC_YUV888:
		FLIP_MACRO(unsigned char, 3);
		break;
	case BC_RGB_FLOAT:
		FLIP_MACRO(float, 3);
		break;
	case BC_RGB161616:
	case BC_YUV161616:
		FLIP_MACRO(uint16_t, 3);
		break;
	case BC_RGBA8888:
	case BC_YUVA8888:
		FLIP_MACRO(unsigned char, 4);
		break;
	case BC_RGBA_FLOAT:
		FLIP_MACRO(float, 4);
		break;
	case BC_RGBA16161616:
	case BC_YUVA16161616:
	case BC_AYUV16161616:
		FLIP_MACRO(uint16_t, 4);
		break;
	}
}


void FlipMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_string(keyframe->data, MESSAGESIZE);
	output.tag.set_title("FLIP");
	output.append_tag();
	if(config.flip_vertical)
	{
		output.tag.set_title("VERTICAL");
		output.append_tag();
		output.tag.set_title("/VERTICAL");
		output.append_tag();
	}

	if(config.flip_horizontal)
	{
		output.tag.set_title("HORIZONTAL");
		output.append_tag();
		output.tag.set_title("/HORIZONTAL");
		output.append_tag();
	}
	output.tag.set_title("/FLIP");
	output.append_tag();
	output.terminate_string();
// data is now in *text
}

void FlipMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->data, strlen(keyframe->data));

	int result = 0;
	config.flip_vertical = config.flip_horizontal = 0;

	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("VERTICAL"))
			{
				config.flip_vertical = 1;
			}
			else
			if(input.tag.title_is("HORIZONTAL"))
			{
				config.flip_horizontal = 1;
			}
		}
	}
}

void FlipMain::load_defaults()
{
// load the defaults
	defaults = load_defaults_file("flip.rc");

	config.flip_horizontal = defaults->get("FLIP_HORIZONTAL", config.flip_horizontal);
	config.flip_vertical = defaults->get("FLIP_VERTICAL", config.flip_vertical);
}

void FlipMain::save_defaults()
{
	defaults->update("FLIP_HORIZONTAL", config.flip_horizontal);
	defaults->update("FLIP_VERTICAL", config.flip_vertical);
	defaults->save();
}

void FlipMain::handle_opengl()
{
#ifdef HAVE_GL
/* FIXIT
	get_output()->to_texture();
	get_output()->enable_opengl();
	get_output()->init_screen();
	get_output()->bind_texture(0);

	if(config.flip_vertical && !config.flip_horizontal)
	{
		get_output()->draw_texture(0,
			0,
			get_output()->get_w(),
			get_output()->get_h(),
			0,
			get_output()->get_h(),
			get_output()->get_w(),
			0);
	}

	if(config.flip_horizontal && !config.flip_vertical)
	{
		get_output()->draw_texture(0,
			0,
			get_output()->get_w(),
			get_output()->get_h(),
			get_output()->get_w(),
			0,
			0,
			get_output()->get_h());
	}

	if(config.flip_vertical && config.flip_horizontal)
	{
		get_output()->draw_texture(0,
			0,
			get_output()->get_w(),
			get_output()->get_h(),
			get_output()->get_w(),
			get_output()->get_h(),
			0,
			0);
	}

	get_output()->set_opengl_state(VFrame::SCREEN);
	*/
#endif
}
