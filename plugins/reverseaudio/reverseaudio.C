
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

// Old name was "Reverse audio"
#define PLUGIN_TITLE N_("Reverse")
#define PLUGIN_IS_AUDIO
#define PLUGIN_IS_REALTIME
#define PLUGIN_CUSTOM_LOAD_CONFIGURATION
#define PLUGIN_CLASS ReverseAudio
#define PLUGIN_CONFIG_CLASS ReverseAudioConfig
#define PLUGIN_THREAD_CLASS ReverseAudioThread
#define PLUGIN_GUI_CLASS ReverseAudioWindow

#include "pluginmacros.h"

#include "aframe.h"
#include "bchash.h"
#include "filexml.h"
#include "language.h"
#include "pluginaclient.h"
#include "pluginwindow.h"
#include "picon_png.h"

#include <string.h>

class ReverseAudioConfig
{
public:
	ReverseAudioConfig();

	int enabled;
	PLUGIN_CONFIG_CLASS_MEMBERS
};


class ReverseAudioEnabled : public BC_CheckBox
{
public:
	ReverseAudioEnabled(ReverseAudio *plugin,
		int x,
		int y);
	int handle_event();
	ReverseAudio *plugin;
};

class ReverseAudioWindow : public PluginWindow
{
public:
	ReverseAudioWindow(ReverseAudio *plugin, int x, int y);
	~ReverseAudioWindow();

	void update();

	ReverseAudioEnabled *enabled;
	PLUGIN_GUI_CLASS_MEMBERS
};

PLUGIN_THREAD_HEADER

class ReverseAudio : public PluginAClient
{
public:
	ReverseAudio(PluginServer *server);
	~ReverseAudio();

	PLUGIN_CLASS_MEMBERS

	void load_defaults();
	void save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void process_frame(AFrame *);

	ptstime input_pts;
	int fragment_size;
	AFrame input_frame;
};


REGISTER_PLUGIN


ReverseAudioConfig::ReverseAudioConfig()
{
	enabled = 1;
}


ReverseAudioWindow::ReverseAudioWindow(ReverseAudio *plugin, int x, int y)
 : PluginWindow(plugin->gui_string, 
	x,
	y, 
	265,
	60)
{
	x = y = 10;
	add_subwindow(enabled = new ReverseAudioEnabled(plugin, 
		x, 
		y));
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

ReverseAudioWindow::~ReverseAudioWindow()
{
}

void ReverseAudioWindow::update()
{
	enabled->update(plugin->config.enabled);
}

PLUGIN_THREAD_METHODS

ReverseAudioEnabled::ReverseAudioEnabled(ReverseAudio *plugin, 
	int x, 
	int y)
 : BC_CheckBox(x, 
	y, 
	plugin->config.enabled,
	_("Enabled"))
{
	this->plugin = plugin;
}

int ReverseAudioEnabled::handle_event()
{
	plugin->config.enabled = get_value();
	plugin->send_configure_change();
	return 1;
}


ReverseAudio::ReverseAudio(PluginServer *server)
 : PluginAClient(server)
{
	PLUGIN_CONSTRUCTOR_MACRO
}


ReverseAudio::~ReverseAudio()
{
	PLUGIN_DESTRUCTOR_MACRO
}

PLUGIN_CLASS_METHODS

void ReverseAudio::process_frame(AFrame *aframe)
{
	input_frame.set_samplerate(aframe->get_samplerate());

	while((fragment_size = aframe->fill_length()) > 0)
	{
		load_configuration();
		input_frame.set_buffer(&aframe->buffer[aframe->get_length()], fragment_size);

		if(config.enabled)
			input_frame.set_fill_request(input_pts, fragment_size);
		else
			input_frame.set_fill_request(aframe->get_end_pts(), fragment_size);
		get_frame(&input_frame);

		aframe->set_filled(aframe->get_length() + input_frame.get_length());

		if(config.enabled)
		{
			double *buffer = input_frame.buffer;
			for(int start = 0, end = fragment_size - 1;
				end > start; 
				start++, end--)
			{
				double temp = buffer[start];
				buffer[start] = buffer[end];
				buffer[end] = temp;
			}
		}
	}
}

int ReverseAudio::load_configuration()
{
	KeyFrame *prev_keyframe, *next_keyframe;

	next_keyframe = next_keyframe_pts(source_pts);
	prev_keyframe = prev_keyframe_pts(source_pts);

// Previous keyframe stays in config object.
	read_data(prev_keyframe);

	ptstime prev_pts = prev_keyframe->pos_time;
	ptstime next_pts = next_keyframe->pos_time;

// Defeat default keyframe
	if(prev_pts == 0 && next_pts == 0)
	{
		next_pts = prev_pts = source_start_pts;
	}

// Get range to flip in requested rate
	ptstime range_start = prev_pts;
	ptstime range_end = next_pts;

// Between keyframe and edge of range or no keyframes
	if(PTSEQU(range_start, range_end))
	{
// Between first keyframe and start of effect
		if(source_pts >= source_start_pts &&
			source_pts < range_start)
		{
			range_start = source_start_pts;
		}
		else
// Between last keyframe and end of effect
		if(source_pts >= range_start &&
			source_pts < source_start_pts + total_len_pts)
		{
			range_end = source_start_pts + total_len_pts;
		}
	}

// Plugin is active
	if(input_frame.get_samplerate() > 0)
	{
		samplenum range_size = input_frame.to_samples(range_end - source_pts);
		if(range_size < fragment_size)
			fragment_size = range_size;
		input_pts = range_end - (source_pts - range_start)
			- input_frame.to_duration(fragment_size);
	}
	return 1;
}

void ReverseAudio::load_defaults()
{
// load the defaults
	defaults = load_defaults_file("reverseaudio.rc");

	config.enabled = defaults->get("ENABLED", config.enabled);
}

void ReverseAudio::save_defaults()
{
	defaults->update("ENABLED", config.enabled);
	defaults->save();
}

void ReverseAudio::save_data(KeyFrame *keyframe)
{
	FileXML output;

	output.tag.set_title("REVERSEAUDIO");
	output.tag.set_property("ENABLED", config.enabled);
	output.append_tag();
	output.tag.set_title("/REVERSEAUDIO");
	output.append_tag();
	keyframe->set_data(output.string);
}

void ReverseAudio::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());

	int result = 0;

	while(!input.read_tag())
	{
		if(input.tag.title_is("REVERSEAUDIO"))
		{
			config.enabled = input.tag.get_property("ENABLED", config.enabled);
		}
	}
}
