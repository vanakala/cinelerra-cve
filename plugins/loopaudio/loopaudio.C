
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
#include "filexml.h"
#include "guicast.h"
#include "language.h"
#include "pluginaclient.h"
#include "picon_png.h"

#include <string.h>

class LoopAudio;

class LoopAudioConfig
{
public:
	LoopAudioConfig();
	ptstime duration;
};


class LoopAudioDuration : public BC_TextBox
{
public:
	LoopAudioDuration(LoopAudio *plugin,
		int x,
		int y);
	int handle_event();
	LoopAudio *plugin;
};

class LoopAudioWindow : public BC_Window
{
public:
	LoopAudioWindow(LoopAudio *plugin, int x, int y);
	~LoopAudioWindow();
	void create_objects();

	LoopAudio *plugin;
	LoopAudioDuration *duration;
};

PLUGIN_THREAD_HEADER(LoopAudio, LoopAudioThread, LoopAudioWindow)

class LoopAudio : public PluginAClient
{
public:
	LoopAudio(PluginServer *server);
	~LoopAudio();

	PLUGIN_CLASS_MEMBERS(LoopAudioConfig, LoopAudioThread)

	void load_defaults();
	void save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void update_gui();
	int is_realtime();
	int is_synthesis();
	int has_pts_api();
	void process_frame(AFrame *aframe);

	AFrame loop_frame;
};


REGISTER_PLUGIN(LoopAudio);

LoopAudioConfig::LoopAudioConfig()
{
	duration = 1;
}


LoopAudioWindow::LoopAudioWindow(LoopAudio *plugin, int x, int y)
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
	this->plugin = plugin;
}

LoopAudioWindow::~LoopAudioWindow()
{
}

void LoopAudioWindow::create_objects()
{
	int x = 10, y = 10;

	set_icon(new VFrame(picon_png));
	add_subwindow(new BC_Title(x, y, _("Duration to loop:")));
	y += 20;
	add_subwindow(duration = new LoopAudioDuration(plugin, 
		x, 
		y));
	show_window();
	flush();
}

PLUGIN_THREAD_OBJECT(LoopAudio, LoopAudioThread, LoopAudioWindow)


LoopAudioDuration::LoopAudioDuration(LoopAudio *plugin, 
	int x, 
	int y)
 : BC_TextBox(x, 
	y, 
	100,
	1,
	(float)plugin->config.duration,
	1,
	MEDIUMFONT,
	2)
{
	this->plugin = plugin;
}

int LoopAudioDuration::handle_event()
{
	plugin->config.duration = atof(get_text());
	plugin->config.duration = MAX(0.01, plugin->config.duration);
	plugin->send_configure_change();
	return 1;
}


LoopAudio::LoopAudio(PluginServer *server)
 : PluginAClient(server)
{
	PLUGIN_CONSTRUCTOR_MACRO
}

LoopAudio::~LoopAudio()
{
	PLUGIN_DESTRUCTOR_MACRO
}

const char* LoopAudio::plugin_title() { return N_("Loop audio"); }
int LoopAudio::is_realtime() { return 1; } 
int LoopAudio::is_synthesis() { return 1; }
int LoopAudio::has_pts_api() { return 1; }

NEW_PICON_MACRO(LoopAudio)
SHOW_GUI_MACRO(LoopAudio, LoopAudioThread)
RAISE_WINDOW_MACRO(LoopAudio)
SET_STRING_MACRO(LoopAudio);

void LoopAudio::process_frame(AFrame *aframe)
{
	ptstime current_pts;
	int fragment_size;

	while((fragment_size = aframe->fill_length()) > 0)
	{
		current_pts = aframe->pts + aframe->duration;

// Truncate to next keyframe
		KeyFrame *next_keyframe = next_keyframe_pts(current_pts);
		ptstime next_pts = next_keyframe->pos_time;
		if(next_pts > current_pts)
		{
			samplenum new_size = aframe->to_samples(next_pts - current_pts);
			fragment_size = MIN(fragment_size, new_size);
		}

// Get start of current loop
		KeyFrame *prev_keyframe = prev_keyframe_pts(current_pts);
		ptstime prev_pts = prev_keyframe->pos_time;
		if(prev_pts == 0)
			prev_pts = source_start_pts;
		read_data(prev_keyframe);

// Get start of fragment in current loop
		double p, q;
		q  = modf((current_pts - prev_pts) / config.duration, &p); // - n.m perioodi

// Check almost full period
		if(aframe->ptsequ(q, 1.0))
			q = 0;

		ptstime loop_pts = prev_pts + q * config.duration;

// Truncate fragment to end of loop
		int left_samples = aframe->to_samples((1 - q) * config.duration);
		if(left_samples > 0)
			fragment_size = MIN(left_samples, fragment_size);
		loop_frame.set_buffer(&aframe->buffer[aframe->length], fragment_size);
		loop_frame.set_fill_request(loop_pts, fragment_size);
		get_aframe_rt(&loop_frame);
		aframe->set_filled(aframe->length + loop_frame.length);
	}
}


int LoopAudio::load_configuration()
{
	KeyFrame *prev_keyframe;
	ptstime old_pts = config.duration;
	prev_keyframe = prev_keyframe_pts(source_pts);
	read_data(prev_keyframe);
	return !PTSEQU(old_pts, config.duration);
}

void LoopAudio::load_defaults()
{
	samplenum samples = 0;
// load the defaults
	defaults = load_defaults_file("loopaudio.rc");
	samples = defaults->get("SAMPLES", samples);
	if(samples > 0)
	{
		int rate = get_project_samplerate();
// Check if we have reasonable samplerate
		if(rate > 1)
			config.duration = (ptstime)samples / get_project_samplerate();
	}
	config.duration = defaults->get("DURATION", config.duration);
}

void LoopAudio::save_defaults()
{
	defaults->update("DURATION", config.duration);
	defaults->save();
}

void LoopAudio::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_string(keyframe->data, MESSAGESIZE);
	output.tag.set_title("LOOPAUDIO");
	output.tag.set_property("DURATION", config.duration);
	output.append_tag();
	output.tag.set_title("/LOOPAUDIO");
	output.append_tag();
	output.terminate_string();
}

void LoopAudio::read_data(KeyFrame *keyframe)
{
	FileXML input;
	samplenum samples = 0;

	input.set_shared_string(keyframe->data, strlen(keyframe->data));

	while(!input.read_tag())
	{
		if(input.tag.title_is("LOOPAUDIO"))
		{
			samples = input.tag.get_property("SAMPLES", samples);
			if(samples > 0)
				config.duration = (ptstime)samples / get_project_samplerate();
			config.duration = input.tag.get_property("DURATION", config.duration);
		}
	}
}

void LoopAudio::update_gui()
{
	if(thread)
	{
		load_configuration();
		thread->window->lock_window("LoopAudio::update_gui");
		thread->window->duration->update((float)config.duration);
		thread->window->unlock_window();
	}
}
