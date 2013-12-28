
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

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME
#define PLUGIN_CUSTOM_LOAD_CONFIGURATION

#define PLUGIN_TITLE N_("Interpolate")
#define PLUGIN_CLASS InterpolateVideo
#define PLUGIN_CONFIG_CLASS InterpolateVideoConfig
#define PLUGIN_THREAD_CLASS InterpolateVideoThread
#define PLUGIN_GUI_CLASS InterpolateVideoWindow

#include "pluginmacros.h"

#include "clip.h"
#include "bchash.h"
#include "filexml.h"
#include "guicast.h"
#include "keyframe.h"
#include "language.h"
#include "picon_png.h"
#include "pluginvclient.h"
#include "theme.h"
#include "vframe.h"

#include <string.h>
#include <stdint.h>


class InterpolateVideoConfig
{
public:
	InterpolateVideoConfig();

	void copy_from(InterpolateVideoConfig *config);
	int equivalent(InterpolateVideoConfig *config);

// Frame rate of input
	double input_rate;
// If 1, use the keyframes as beginning and end frames and ignore input rate
	int use_keyframes;
	PLUGIN_CONFIG_CLASS_MEMBERS
};


class InterpolateVideoRate : public BC_TextBox
{
public:
	InterpolateVideoRate(InterpolateVideo *plugin, 
		InterpolateVideoWindow *gui, 
		int x, 
		int y);
	int handle_event();
	InterpolateVideo *plugin;
	InterpolateVideoWindow *gui;
};

class InterpolateVideoRateMenu : public BC_ListBox
{
public:
	InterpolateVideoRateMenu(InterpolateVideo *plugin, 
		InterpolateVideoWindow *gui, 
		int x, 
		int y);
	int handle_event();
	InterpolateVideo *plugin;
	InterpolateVideoWindow *gui;
};

class InterpolateVideoKeyframes : public BC_CheckBox
{
public:
	InterpolateVideoKeyframes(InterpolateVideo *plugin,
		InterpolateVideoWindow *gui,
		int x, 
		int y);
	int handle_event();
	InterpolateVideoWindow *gui;
	InterpolateVideo *plugin;
};

class InterpolateVideoWindow : public BC_Window
{
public:
	InterpolateVideoWindow(InterpolateVideo *plugin, int x, int y);
	~InterpolateVideoWindow();

	void update();
	void update_enabled();

	ArrayList<BC_ListBoxItem*> frame_rates;

	InterpolateVideoRate *rate;
	InterpolateVideoRateMenu *rate_menu;
	InterpolateVideoKeyframes *keyframes;
	PLUGIN_GUI_CLASS_MEMBERS
};


PLUGIN_THREAD_HEADER


class InterpolateVideo : public PluginVClient
{
public:
	InterpolateVideo(PluginServer *server);
	~InterpolateVideo();

	PLUGIN_CLASS_MEMBERS

	void process_frame(VFrame *frame);
	void load_defaults();
	void save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);

	void fill_border(double frame_rate, ptstime start_pts);

// beginning and end frames
	VFrame *frames[2];

// Current requested positions
	ptstime range_start_pts;
	ptstime range_end_pts;
};


InterpolateVideoConfig::InterpolateVideoConfig()
{
	input_rate = (double)30000 / 1001;
	use_keyframes = 0;
}

void InterpolateVideoConfig::copy_from(InterpolateVideoConfig *config)
{
	this->input_rate = config->input_rate;
	this->use_keyframes = config->use_keyframes;
}

int InterpolateVideoConfig::equivalent(InterpolateVideoConfig *config)
{
	return EQUIV(this->input_rate, config->input_rate) &&
		(this->use_keyframes == config->use_keyframes);
}


InterpolateVideoWindow::InterpolateVideoWindow(InterpolateVideo *plugin, int x, int y)
 : BC_Window(plugin->gui_string, 
	x,
	y, 
	210, 
	160, 
	200, 
	160, 
	0, 
	0,
	1)
{
	BC_Title *title;
	x = y = 10;

	add_subwindow(title = new BC_Title(x, y, _("Input frames per second:")));
	y += 30;
	add_subwindow(rate = new InterpolateVideoRate(plugin, 
		this, 
		x, 
		y));
	add_subwindow(rate_menu = new InterpolateVideoRateMenu(plugin, 
		this, 
		x + rate->get_w() + 5, 
		y));
	y += 30;
	add_subwindow(keyframes = new InterpolateVideoKeyframes(plugin,
		this,
		x, 
		y));
	PLUGIN_GUI_CONSTRUCTOR_MACRO
	update_enabled();
}

InterpolateVideoWindow::~InterpolateVideoWindow()
{
}

void InterpolateVideoWindow::update()
{
	rate->update((float)plugin->config.input_rate);
	keyframes->update(plugin->config.use_keyframes);
	update_enabled();
}

void InterpolateVideoWindow::update_enabled()
{
	if(plugin->config.use_keyframes)
	{
		rate->disable();
	}
	else
	{
		rate->enable();
	}
}


InterpolateVideoRate::InterpolateVideoRate(InterpolateVideo *plugin, 
	InterpolateVideoWindow *gui, 
	int x, 
	int y)
 : BC_TextBox(x, 
	y, 
	90,
	1,
	(float)plugin->config.input_rate)
{
	this->plugin = plugin;
	this->gui = gui;
}

int InterpolateVideoRate::handle_event()
{
	plugin->config.input_rate = Units::atoframerate(get_text());
	plugin->send_configure_change();
	return 1;
}


InterpolateVideoRateMenu::InterpolateVideoRateMenu(InterpolateVideo *plugin, 
	InterpolateVideoWindow *gui, 
	int x, 
	int y)
 : BC_ListBox(x,
	y,
	100,
	200,
	&plugin->get_theme()->frame_rates,
	LISTBOX_POPUP)
{
	this->plugin = plugin;
	this->gui = gui;
}

int InterpolateVideoRateMenu::handle_event()
{
	char *text = get_selection(0, 0)->get_text();
	plugin->config.input_rate = atof(text);
	gui->rate->update(text);
	plugin->send_configure_change();
	return 1;
}


InterpolateVideoKeyframes::InterpolateVideoKeyframes(InterpolateVideo *plugin,
	InterpolateVideoWindow *gui,
	int x, 
	int y)
 : BC_CheckBox(x, 
	y, 
	plugin->config.use_keyframes, 
	_("Use keyframes as input"))
{
	this->plugin = plugin;
	this->gui = gui;
}

int InterpolateVideoKeyframes::handle_event()
{
	plugin->config.use_keyframes = get_value();
	gui->update_enabled();
	plugin->send_configure_change();
	return 1;
}


PLUGIN_THREAD_METHODS
REGISTER_PLUGIN


InterpolateVideo::InterpolateVideo(PluginServer *server)
 : PluginVClient(server)
{
	for(int i = 0; i < 2; i++)
		frames[i] = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}


InterpolateVideo::~InterpolateVideo()
{
	if(frames[0]) delete frames[0];
	if(frames[1]) delete frames[1];
	PLUGIN_DESTRUCTOR_MACRO
}

PLUGIN_CLASS_METHODS

void InterpolateVideo::fill_border(double frame_rate, ptstime start_pts)
{
	if(!frames[0]->pts_in_frame(range_start_pts))
	{
		frames[0]->set_pts(range_start_pts);
		get_frame(frames[0]);
	}

	if(!frames[1]->pts_in_frame(range_end_pts))
	{
		frames[1]->set_pts(range_end_pts);
		get_frame(frames[1]);
	}
}


#define AVERAGE(type, temp_type,components, max) \
{ \
	temp_type fraction0 = (temp_type)(lowest_fraction * max); \
	temp_type fraction1 = (temp_type)(max - fraction0); \
 \
	for(int i = 0; i < h; i++) \
	{ \
		type *in_row0 = (type*)frames[0]->get_rows()[i]; \
		type *in_row1 = (type*)frames[1]->get_rows()[i]; \
		type *out_row = (type*)frame->get_rows()[i]; \
		for(int j = 0; j < w * components; j++) \
		{ \
			*out_row++ = (*in_row0++ * fraction0 + *in_row1++ * fraction1) / max; \
		} \
	} \
}


void InterpolateVideo::process_frame(VFrame *frame)
{
	load_configuration();

	if(!frames[0])
	{
		for(int i = 0; i < 2; i++)
		{
			frames[i] = new VFrame(0,
				frame->get_w(),
				frame->get_h(),
				frame->get_color_model(),
				-1);
		}
	}

	if(PTSEQU(range_start_pts, range_end_pts))
	{
		get_frame(frame);
		return;
	}
	else
	{

// Fill border frames
		fill_border(frame_rate, frame->get_pts());
// Fraction of lowest frame in output
		float highest_fraction = (frame->get_pts() - frames[0]->get_pts()) /
			(frames[1]->get_pts() - frames[0]->get_pts());
// Fraction of highest frame in output
		float lowest_fraction = 1.0 - highest_fraction;
		CLAMP(highest_fraction, 0, 1);
		CLAMP(lowest_fraction, 0, 1);

		int w = frame->get_w();
		int h = frame->get_h();

		switch(frame->get_color_model())
		{
		case BC_RGB_FLOAT:
			AVERAGE(float, float, 3, 1);
			break;
		case BC_RGB888:
		case BC_YUV888:
			AVERAGE(unsigned char, int, 3, 0xff);
			break;
		case BC_RGBA_FLOAT:
			AVERAGE(float, float, 4, 1);
			break;
		case BC_RGBA8888:
		case BC_YUVA8888:
			AVERAGE(unsigned char, int, 4, 0xff);
			break;
		case BC_RGB161616:
		case BC_YUV161616:
			AVERAGE(uint16_t, int, 3, 0xffff);
			break;
		case BC_RGBA16161616:
		case BC_YUVA16161616:
			AVERAGE(uint16_t, int, 4, 0xffff);
			break;
		}
	}
}

int InterpolateVideo::load_configuration()
{
	KeyFrame *prev_keyframe, *next_keyframe;
	InterpolateVideoConfig old_config;
	double active_input_rate;

	old_config.copy_from(&config);

	next_keyframe = next_keyframe_pts(source_pts);
	prev_keyframe = prev_keyframe_pts(source_pts);

// Previous keyframe stays in config object.
	read_data(prev_keyframe);

	ptstime prev_pts = prev_keyframe->pos_time;
	ptstime next_pts = next_keyframe->pos_time;
	if(prev_pts < EPSILON && next_pts < EPSILON)
	{
		next_pts = prev_pts = source_start_pts;
	}

// Get range to average in requested rate
	range_start_pts = prev_pts;
	range_end_pts = next_pts;

// Use keyframes to determine range
	if(config.use_keyframes)
	{
		active_input_rate = get_project_framerate();
// Between keyframe and edge of range or no keyframes
		if(PTSEQU(range_start_pts, range_end_pts))
		{
// Between first keyframe and start of effect
			if(source_pts >= source_start_pts &&
				source_pts < range_start_pts)
			{
				range_start_pts = source_start_pts;
			}
			else
// Between last keyframe and end of effect
			if(source_pts >= range_start_pts &&
				source_pts < source_start_pts + total_len_pts)
			{
// Last frame should be inclusive of current effect
				range_end_pts = source_start_pts + total_len_pts - 1 / active_input_rate;
			}
		}
	}
	else
// Use frame rate
	{
		active_input_rate = config.input_rate;
// Convert to input frame rate
		range_start_pts = floor((source_pts - source_start_pts) * active_input_rate)
				/ active_input_rate + source_start_pts;
		range_end_pts = source_pts + 1.0 / active_input_rate;
	}
	return !config.equivalent(&old_config);
}

void InterpolateVideo::load_defaults()
{
	defaults = load_defaults_file("interpolatevideo.rc");

	config.input_rate = defaults->get("INPUT_RATE", config.input_rate);
	config.input_rate = Units::fix_framerate(config.input_rate);
	config.use_keyframes = defaults->get("USE_KEYFRAMES", config.use_keyframes);
}

void InterpolateVideo::save_defaults()
{
	defaults->update("INPUT_RATE", config.input_rate);
	defaults->update("USE_KEYFRAMES", config.use_keyframes);
	defaults->save();
}

void InterpolateVideo::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_string(keyframe->data, MESSAGESIZE);
	output.tag.set_title("INTERPOLATEVIDEO");
	output.tag.set_property("INPUT_RATE", config.input_rate);
	output.tag.set_property("USE_KEYFRAMES", config.use_keyframes);
	output.append_tag();
	output.tag.set_title("/INTERPOLATEVIDEO");
	output.append_tag();
	output.terminate_string();
}

void InterpolateVideo::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->data, strlen(keyframe->data));

	while(!input.read_tag())
	{
		if(input.tag.title_is("INTERPOLATEVIDEO"))
		{
			config.input_rate = input.tag.get_property("INPUT_RATE", config.input_rate);
			config.input_rate = Units::fix_framerate(config.input_rate);
			config.use_keyframes = input.tag.get_property("USE_KEYFRAMES", config.use_keyframes);
		}
	}
}
