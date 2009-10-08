
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
#include "denoisevideo.h"
#include "filexml.h"
#include "guicast.h"
#include "keyframe.h"
#include "language.h"
#include "picon_png.h"
#include "vframe.h"





#include <stdint.h>
#include <string.h>




REGISTER_PLUGIN(DenoiseVideo)






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
	long prev_frame, 
	long next_frame, 
	long current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);

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
	DenoiseVideoWindow *gui, 
	int x, 
	int y, 
	int *output,
	char *text)
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
 : BC_Window(plugin->gui_string, 
 	x, 
	y, 
	210, 
	240, 
	200, 
	240, 
	0, 
	0,
	1)
{
	this->plugin = plugin;
}


void DenoiseVideoWindow::create_objects()
{
	int x = 10, y = 10;
	add_subwindow(new BC_Title(x, y, _("Frames to accumulate:")));
	y += 20;
	add_subwindow(frames = new DenoiseVideoFrames(plugin, x, y));
	y += 30;
	add_subwindow(new BC_Title(x, y, _("Threshold:")));
	y += 20;
	add_subwindow(threshold = new DenoiseVideoThreshold(plugin, x, y));
	y += 40;
	add_subwindow(do_r = new DenoiseVideoToggle(plugin, this, x, y, &plugin->config.do_r, _("Red")));
	y += 30;
	add_subwindow(do_g = new DenoiseVideoToggle(plugin, this, x, y, &plugin->config.do_g, _("Green")));
	y += 30;
	add_subwindow(do_b = new DenoiseVideoToggle(plugin, this, x, y, &plugin->config.do_b, _("Blue")));
	y += 30;
	add_subwindow(do_a = new DenoiseVideoToggle(plugin, this, x, y, &plugin->config.do_a, _("Alpha")));
	show_window();
	flush();
}

int DenoiseVideoWindow::close_event()
{
	set_done(1);
	return 1;
}






PLUGIN_THREAD_OBJECT(DenoiseVideo, DenoiseVideoThread, DenoiseVideoWindow)











DenoiseVideo::DenoiseVideo(PluginServer *server)
 : PluginVClient(server)
{
	PLUGIN_CONSTRUCTOR_MACRO
	accumulation = 0;
}


DenoiseVideo::~DenoiseVideo()
{
	PLUGIN_DESTRUCTOR_MACRO

	if(accumulation) delete [] accumulation;
}

int DenoiseVideo::process_realtime(VFrame *input, VFrame *output)
{
	load_configuration();

	int h = input->get_h();
	int w = input->get_w();
	int color_model = input->get_color_model();

	if(!accumulation)
	{
		accumulation = new float[w * h * cmodel_components(color_model)];
		bzero(accumulation, sizeof(float) * w * h * cmodel_components(color_model));
	}

	float *accumulation_ptr = accumulation;
	float opacity = (float)1.0 / config.frames;
	float transparency = 1 - opacity;
	float threshold = (float)config.threshold * 
		cmodel_calculate_max(color_model);
	int do_it[4] = { config.do_r, config.do_g, config.do_b, config.do_a };

#define DENOISE_MACRO(type, components, max) \
{ \
	for(int i = 0; i < h; i++) \
	{ \
		type *output_row = (type*)output->get_rows()[i]; \
		type *input_row = (type*)input->get_rows()[i]; \
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
	}
}


char* DenoiseVideo::plugin_title() { return N_("Denoise video"); }
int DenoiseVideo::is_realtime() { return 1; }


NEW_PICON_MACRO(DenoiseVideo)

SHOW_GUI_MACRO(DenoiseVideo, DenoiseVideoThread)

RAISE_WINDOW_MACRO(DenoiseVideo)

SET_STRING_MACRO(DenoiseVideo);

LOAD_CONFIGURATION_MACRO(DenoiseVideo, DenoiseVideoConfig)

void DenoiseVideo::update_gui()
{
	if(thread)
	{
		load_configuration();
		thread->window->lock_window();
		thread->window->frames->update(config.frames);
		thread->window->threshold->update(config.threshold);
		thread->window->unlock_window();
	}
}



int DenoiseVideo::load_defaults()
{
	char directory[BCTEXTLEN];
// set the default directory
	sprintf(directory, "%sdenoisevideo.rc", BCASTDIR);

// load the defaults
	defaults = new BC_Hash(directory);
	defaults->load();

	config.frames = defaults->get("FRAMES", config.frames);
	config.threshold = defaults->get("THRESHOLD", config.threshold);
	config.do_r = defaults->get("DO_R", config.do_r);
	config.do_g = defaults->get("DO_G", config.do_g);
	config.do_b = defaults->get("DO_B", config.do_b);
	config.do_a = defaults->get("DO_A", config.do_a);
	return 0;
}

int DenoiseVideo::save_defaults()
{
	defaults->update("THRESHOLD", config.threshold);
	defaults->update("FRAMES", config.frames);
	defaults->update("DO_R", config.do_r);
	defaults->update("DO_G", config.do_g);
	defaults->update("DO_B", config.do_b);
	defaults->update("DO_A", config.do_a);
	defaults->save();
	return 0;
}

void DenoiseVideo::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_string(keyframe->data, MESSAGESIZE);
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
	output.terminate_string();
}

void DenoiseVideo::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->data, strlen(keyframe->data));

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




