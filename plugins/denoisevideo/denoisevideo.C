
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

#include "bctitle.h"
#include "clip.h"
#include "bchash.h"
#include "denoisevideo.h"
#include "filexml.h"
#include "keyframe.h"
#include "language.h"
#include "picon_png.h"
#include "vframe.h"

#include <stdint.h>
#include <string.h>

REGISTER_PLUGIN


DenoiseVideoConfig::DenoiseVideoConfig()
{
	frames = 2;
	threshold = 0.1;
	do_r = 1;
	do_g = 1;
	do_b = 1;
	do_a = 1;
}

int DenoiseVideoConfig::equivalent(DenoiseVideoConfig &that)
{
	return frames == that.frames && 
		EQUIV(threshold, that.threshold) &&
		do_r == that.do_r &&
		do_g == that.do_g &&
		do_b == that.do_b &&
		do_a == that.do_a;
}

void DenoiseVideoConfig::copy_from(DenoiseVideoConfig &that)
{
	frames = that.frames;
	threshold = that.threshold;
	do_r = that.do_r;
	do_g = that.do_g;
	do_b = that.do_b;
	do_a = that.do_a;
}

void DenoiseVideoConfig::interpolate(DenoiseVideoConfig &prev, 
	DenoiseVideoConfig &next, 
	ptstime prev_pts,
	ptstime next_pts,
	ptstime current_pts)
{
	PLUGIN_CONFIG_INTERPOLATE_MACRO

	this->frames = (int)(prev.frames * prev_scale + next.frames * next_scale);
	this->threshold = prev.threshold * prev_scale + next.threshold * next_scale;
	do_r = prev.do_r;
	do_g = prev.do_g;
	do_b = prev.do_b;
	do_a = prev.do_a;
}


DenoiseVideoFrames::DenoiseVideoFrames(DenoiseVideo *plugin, int x, int y)
 : BC_ISlider(x, 
	y, 
	0,
	190, 
	200, 
	1, 
	256, 
	plugin->config.frames)
{
	this->plugin = plugin;
}

int DenoiseVideoFrames::handle_event()
{
	int result = get_value();
	if(result < 1 || result > 256) result = 256;
	plugin->config.frames = result;
	plugin->send_configure_change();
	return 1;
}


DenoiseVideoThreshold::DenoiseVideoThreshold(DenoiseVideo *plugin, int x, int y)
 : BC_TextBox(x, y, 100, 1, plugin->config.threshold)
{
	this->plugin = plugin;
}

int DenoiseVideoThreshold::handle_event()
{
	plugin->config.threshold = atof(get_text());
	plugin->send_configure_change();
	return 1;
}


DenoiseVideoToggle::DenoiseVideoToggle(DenoiseVideo *plugin, 
	int x, 
	int y, 
	int *output,
	const char *text)
 : BC_CheckBox(x, y, *output, text)
{
	this->plugin = plugin;
	this->output = output;
}

int DenoiseVideoToggle::handle_event()
{
	*output = get_value();
	plugin->send_configure_change();
}


DenoiseVideoWindow::DenoiseVideoWindow(DenoiseVideo *plugin, int x, int y)
 : PluginWindow(plugin->gui_string, 
	x,
	y, 
	210, 
	240)
{
	x = y = 10;

	add_subwindow(new BC_Title(x, y, _("Frames to accumulate:")));
	y += 20;
	add_subwindow(frames = new DenoiseVideoFrames(plugin, x, y));
	y += 30;
	add_subwindow(new BC_Title(x, y, _("Threshold:")));
	y += 20;
	add_subwindow(threshold = new DenoiseVideoThreshold(plugin, x, y));
	y += 40;
	add_subwindow(do_r = new DenoiseVideoToggle(plugin, x, y, &plugin->config.do_r, _("Red")));
	y += 30;
	add_subwindow(do_g = new DenoiseVideoToggle(plugin, x, y, &plugin->config.do_g, _("Green")));
	y += 30;
	add_subwindow(do_b = new DenoiseVideoToggle(plugin, x, y, &plugin->config.do_b, _("Blue")));
	y += 30;
	add_subwindow(do_a = new DenoiseVideoToggle(plugin, x, y, &plugin->config.do_a, _("Alpha")));
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

void DenoiseVideoWindow::update()
{
	frames->update(plugin->config.frames);
	threshold->update(plugin->config.threshold);
}


PLUGIN_THREAD_METHODS


DenoiseVideo::DenoiseVideo(PluginServer *server)
 : PluginVClient(server)
{
	accumulation = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

DenoiseVideo::~DenoiseVideo()
{
	if(accumulation) delete [] accumulation;
	PLUGIN_DESTRUCTOR_MACRO
}

PLUGIN_CLASS_METHODS

VFrame *DenoiseVideo::process_tmpframe(VFrame *input)
{
	load_configuration();

	int h = input->get_h();
	int w = input->get_w();
	int color_model = input->get_color_model();

	if(!accumulation)
	{
		accumulation = new float[w * h * ColorModels::components(color_model)];
		memset(accumulation, 0, sizeof(float) * w * h * ColorModels::components(color_model));
	}

	float *accumulation_ptr = accumulation;
	float opacity = (float)1.0 / config.frames;
	float transparency = 1 - opacity;
	float threshold = (float)config.threshold * 
		ColorModels::calculate_max(color_model);
	int do_it[4] = { config.do_r, config.do_g, config.do_b, config.do_a };

#define DENOISE_MACRO(type, components, max) \
{ \
	for(int i = 0; i < h; i++) \
	{ \
		type *output_row = (type*)input->get_row_ptr(i); \
		type *input_row = (type*)input->get_row_ptr(i); \
 \
		for(int k = 0; k < w * components; k++) \
		{ \
			if(do_it[k % components]) \
			{ \
				float input_pixel = *input_row; \
				(*accumulation_ptr) = \
					transparency * (*accumulation_ptr) + \
					opacity * input_pixel; \
 \
				if(fabs((*accumulation_ptr) - input_pixel) > threshold) \
				{ \
					(*accumulation_ptr) = input_pixel; \
					*output_row = (type)(*accumulation_ptr); \
				} \
				else \
				if(sizeof(type) < 4) \
					*output_row = (type)CLIP((*accumulation_ptr), 0, max); \
			} \
			else \
			{ \
				*output_row = *input_row; \
			} \
 \
			output_row++; \
			input_row++; \
			accumulation_ptr++; \
		} \
	} \
}

	switch(color_model)
	{
	case BC_RGB888:
	case BC_YUV888:
		DENOISE_MACRO(unsigned char, 3, 0xff);
		break;

	case BC_RGB_FLOAT:
		DENOISE_MACRO(float, 3, 1.0);
		break;

	case BC_RGBA8888:
	case BC_YUVA8888:
		DENOISE_MACRO(unsigned char, 4, 0xff);
		break;

	case BC_RGBA_FLOAT:
		DENOISE_MACRO(float, 4, 1.0);
		break;

	case BC_RGB161616:
	case BC_YUV161616:
		DENOISE_MACRO(uint16_t, 3, 0xffff);
		break;

	case BC_RGBA16161616:
	case BC_YUVA16161616:
		DENOISE_MACRO(uint16_t, 4, 0xffff);
		break;

	case BC_AYUV16161616:
		// rotate do_it
		int t = do_it[3];
		for(int i = 3; i > 0; i--)
			do_it[i] = do_it[i - 1];
		do_it[0] = t;
		DENOISE_MACRO(uint16_t, 4, 0xffff);
		break;
	}
	if(config.do_a && ColorModels::has_alpha(input->get_color_model()))
		input->set_transparent();
	return input;
}

void DenoiseVideo::load_defaults()
{
	defaults = load_defaults_file("denoisevideo.rc");

	config.frames = defaults->get("FRAMES", config.frames);
	config.threshold = defaults->get("THRESHOLD", config.threshold);
	config.do_r = defaults->get("DO_R", config.do_r);
	config.do_g = defaults->get("DO_G", config.do_g);
	config.do_b = defaults->get("DO_B", config.do_b);
	config.do_a = defaults->get("DO_A", config.do_a);
}

void DenoiseVideo::save_defaults()
{
	defaults->update("THRESHOLD", config.threshold);
	defaults->update("FRAMES", config.frames);
	defaults->update("DO_R", config.do_r);
	defaults->update("DO_G", config.do_g);
	defaults->update("DO_B", config.do_b);
	defaults->update("DO_A", config.do_a);
	defaults->save();
}

void DenoiseVideo::save_data(KeyFrame *keyframe)
{
	FileXML output;

	output.tag.set_title("DENOISE_VIDEO");
	output.tag.set_property("FRAMES", config.frames);
	output.tag.set_property("THRESHOLD", config.threshold);
	output.tag.set_property("DO_R", config.do_r);
	output.tag.set_property("DO_G", config.do_g);
	output.tag.set_property("DO_B", config.do_b);
	output.tag.set_property("DO_A", config.do_a);
	output.append_tag();
	output.tag.set_title("/DENOISE_VIDEO");
	output.append_tag();
	keyframe->set_data(output.string);
}

void DenoiseVideo::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());

	int result = 0;

	while(!input.read_tag())
	{
		if(input.tag.title_is("DENOISE_VIDEO"))
		{
			config.frames = input.tag.get_property("FRAMES", config.frames);
			config.threshold = input.tag.get_property("THRESHOLD", config.threshold);
			config.do_r = input.tag.get_property("DO_R", config.do_r);
			config.do_g = input.tag.get_property("DO_G", config.do_g);
			config.do_b = input.tag.get_property("DO_B", config.do_b);
			config.do_a = input.tag.get_property("DO_A", config.do_a);
		}
	}
}
