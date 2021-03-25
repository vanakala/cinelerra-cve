// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

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
#include <stdlib.h>
#include <string.h>

REGISTER_PLUGIN

const char *DenoiseVideoWindow::denoise_chn_rgba[] =
{
    N_("Red"),
    N_("Green"),
    N_("Blue"),
    N_("Alpha")
};

const char *DenoiseVideoWindow::denoise_chn_ayuv[] =
{
    N_("Alpha"),
    N_("Y"),
    N_("U"),
    N_("V")
};


DenoiseVideoConfig::DenoiseVideoConfig()
{
	frames = 2;
	threshold = 0.1;
	chan0 = 1;
	chan1 = 1;
	chan2 = 1;
	chan3 = 1;
}

int DenoiseVideoConfig::equivalent(DenoiseVideoConfig &that)
{
	return frames == that.frames && 
		EQUIV(threshold, that.threshold) &&
		chan0 == chan0 &&
		chan1 == chan1 &&
		chan2 == chan2 &&
		chan3 == chan3;
}

void DenoiseVideoConfig::copy_from(DenoiseVideoConfig &that)
{
	frames = that.frames;
	threshold = that.threshold;
	chan0 = chan0;
	chan1 = chan1;
	chan2 = chan2;
	chan3 = chan3;
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
	chan0 = prev.chan0;
	chan1 = prev.chan1;
	chan2 = prev.chan2;
	chan3 = prev.chan3;
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

	if(result < 1 || result > 256)
		result = 256;
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
 : PluginWindow(plugin, x, y, 210, 270)
{
	BC_WindowBase *win;
	int cmodel = plugin->get_project_color_model();
	const char **names;
	int title_h;

	x = y = 10;

	if(cmodel == BC_AYUV16161616)
		names = denoise_chn_ayuv;
	else
		names = denoise_chn_rgba;

	add_subwindow(win = print_title(x, y, "%s: %s", plugin->plugin_title(),
		ColorModels::name(cmodel)));
	title_h = win->get_h() + 8;
	y += title_h;
	add_subwindow(new BC_Title(x, y, _("Frames to accumulate:")));
	y += title_h;
	add_subwindow(frames = new DenoiseVideoFrames(plugin, x, y));
	y += frames->get_h() + 8;
	add_subwindow(new BC_Title(x, y, _("Threshold:")));
	y += title_h;
	add_subwindow(threshold = new DenoiseVideoThreshold(plugin, x, y));
	y += threshold->get_h() + 8;
	add_subwindow(chan0 = new DenoiseVideoToggle(plugin, x, y,
		&plugin->config.chan0, _(names[0])));
	y += chan0->get_h() + 8;
	add_subwindow(chan1 = new DenoiseVideoToggle(plugin, x, y,
		&plugin->config.chan1, _(names[1])));
	y += chan1->get_h() + 8;
	add_subwindow(chan2 = new DenoiseVideoToggle(plugin, x, y,
		&plugin->config.chan2, _(names[2])));
	y += chan2->get_h() + 8;
	add_subwindow(chan3 = new DenoiseVideoToggle(plugin, x, y,
		&plugin->config.chan2, _(names[3])));
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
	delete [] accumulation;
	PLUGIN_DESTRUCTOR_MACRO
}

void DenoiseVideo::reset_plugin()
{
	if(accumulation)
		delete [] accumulation;
	accumulation = 0;
}

PLUGIN_CLASS_METHODS

VFrame *DenoiseVideo::process_tmpframe(VFrame *input)
{
	int color_model = input->get_color_model();
	int h = input->get_h();
	int w = input->get_w();

	switch(color_model)
	{
	case BC_RGBA16161616:
		if(config.chan3)
			input->set_transparent();
		break;
	case BC_AYUV16161616:
		if(config.chan0)
			input->set_transparent();
		break;
	default:
		unsupported(color_model);
		return input;
	}

	if(load_configuration())
		update_gui();

	if(!accumulation)
	{
		accumulation = new int[w * h * 4];
		memset(accumulation, 0, sizeof(int) * w * h * 4);
	}

	int *accumulation_ptr = accumulation;
	double opacity = 1.0 / config.frames;
	double transparency = 1 - opacity;
	int threshold = config.threshold * 0xffff;
	int do_it[4] = { config.chan0, config.chan1, config.chan2, config.chan3 };

	switch(color_model)
	{
	case BC_RGBA16161616:
	case BC_AYUV16161616:
		for(int i = 0; i < h; i++)
		{
			uint16_t *input_row = (uint16_t*)input->get_row_ptr(i);

			for(int k = 0; k < w * 4; k++)
			{
				if(do_it[k % 4])
				{
					int input_pixel = *input_row;

					*accumulation_ptr =
						transparency * *accumulation_ptr +
						opacity * input_pixel;

					if(abs(*accumulation_ptr - input_pixel) > threshold)
					{
						*accumulation_ptr = input_pixel;
						*input_row = *accumulation_ptr;
					}
					else
						*input_row = CLIP(*accumulation_ptr, 0, 0xffff);
				}

				input_row++;
				accumulation_ptr++;
			}
		}
		break;
	}
	return input;
}

void DenoiseVideo::load_defaults()
{
	defaults = load_defaults_file("denoisevideo.rc");

	config.frames = defaults->get("FRAMES", config.frames);
	config.threshold = defaults->get("THRESHOLD", config.threshold);
	// Compatibility
	config.chan0 = defaults->get("DO_R", config.chan0);
	config.chan1 = defaults->get("DO_G", config.chan1);
	config.chan2 = defaults->get("DO_B", config.chan2);
	config.chan3 = defaults->get("DO_A", config.chan3);

	config.chan0 = defaults->get("CHAN0", config.chan0);
	config.chan1 = defaults->get("CHAN1", config.chan1);
	config.chan2 = defaults->get("CHAN2", config.chan2);
	config.chan3 = defaults->get("CHAN3", config.chan3);
}

void DenoiseVideo::save_defaults()
{
	defaults->update("THRESHOLD", config.threshold);
	defaults->update("FRAMES", config.frames);
	defaults->delete_key("DO_R");
	defaults->delete_key("DO_G");
	defaults->delete_key("DO_B");
	defaults->delete_key("DO_A");
	defaults->update("CHAN0", config.chan0);
	defaults->update("CHAN1", config.chan1);
	defaults->update("CHAN2", config.chan2);
	defaults->update("CHAN3", config.chan3);
	defaults->save();
}

void DenoiseVideo::save_data(KeyFrame *keyframe)
{
	FileXML output;

	output.tag.set_title("DENOISE_VIDEO");
	output.tag.set_property("FRAMES", config.frames);
	output.tag.set_property("THRESHOLD", config.threshold);
	output.tag.set_property("CHAN0", config.chan0);
	output.tag.set_property("CHAN1", config.chan1);
	output.tag.set_property("CHAN2", config.chan2);
	output.tag.set_property("CHAN3", config.chan3);
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
			// Compatibility
			config.chan0 = input.tag.get_property("DO_R", config.chan0);
			config.chan1 = input.tag.get_property("DO_G", config.chan1);
			config.chan2 = input.tag.get_property("DO_B", config.chan2);
			config.chan3 = input.tag.get_property("DO_A", config.chan3);

			config.chan0 = input.tag.get_property("CHAN0", config.chan0);
			config.chan1 = input.tag.get_property("CHAN1", config.chan1);
			config.chan2 = input.tag.get_property("CHAN2", config.chan2);
			config.chan3 = input.tag.get_property("CHAN3", config.chan3);
		}
	}
}
