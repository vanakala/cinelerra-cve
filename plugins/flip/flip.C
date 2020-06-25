// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

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

VFrame *FlipMain::process_tmpframe(VFrame *frame)
{
	int i, j, k, l;
	int w = frame->get_w();
	int h = frame->get_h();
	int colormodel = frame->get_color_model();
	uint16_t *input_row;
	uint16_t *output_row;
	uint16_t pix;

	if(load_configuration())
		update_gui();

	switch(colormodel)
	{
	case BC_RGBA16161616:
	case BC_AYUV16161616:
		if(config.flip_vertical)
		{
			for(i = 0, j = h - 1; i < h / 2; i++, j--)
			{
				input_row = (uint16_t*)frame->get_row_ptr(i);
				output_row = (uint16_t*)frame->get_row_ptr(j);

				for(k = 0; k < w; k++)
				{
					pix = output_row[0];
					output_row[0] = input_row[0];
					input_row[0] = pix;

					pix = output_row[1];
					output_row[1] = input_row[1];
					input_row[1] = pix;

					pix = output_row[2];
					output_row[2] = input_row[2];
					input_row[2] = pix;

					pix = output_row[3];
					output_row[3] = input_row[3];
					input_row[3] = pix;

					output_row += 4;
					input_row += 4;
				}
			}
		}

		if(config.flip_horizontal)
		{
			for(i = 0; i < h; i++)
			{
				input_row = (uint16_t*)frame->get_row_ptr(i);
				output_row = (uint16_t*)frame->get_row_ptr(i) + (w - 1) * 4;

				for(k = 0; k < w / 2; k++)
				{
					pix = output_row[0];
					output_row[0] = input_row[0];
					input_row[0] = pix;

					pix = output_row[1];
					output_row[1] = input_row[1];
					input_row[1] = pix;

					pix = output_row[2];
					output_row[2] = input_row[2];
					input_row[2] = pix;

					pix = output_row[3];
					output_row[3] = input_row[3];
					input_row[3] = pix;
					input_row += 4;
					output_row -= 4;
				}
			}
		}
		break;
	default:
		unsupported(colormodel);
		break;
	}
	return frame;
}

void FlipMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

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
	keyframe->set_data(output.string);
}

void FlipMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());

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
