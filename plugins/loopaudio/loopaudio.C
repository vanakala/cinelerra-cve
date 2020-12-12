// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "aframe.h"
#include "bchash.h"
#include "bctitle.h"
#include "clip.h"
#include "filexml.h"
#include "loopaudio.h"
#include "picon_png.h"

#include <string.h>


REGISTER_PLUGIN

LoopAudioConfig::LoopAudioConfig()
{
	duration = 1;
}


LoopAudioWindow::LoopAudioWindow(LoopAudio *plugin, int x, int y)
 : PluginWindow(plugin->gui_string, 
	x,
	y, 
	210, 
	160)
{
	x = y = 10;

	add_subwindow(new BC_Title(x, y, _("Duration to loop:")));
	y += 20;
	add_subwindow(duration = new LoopAudioDuration(plugin, 
		x, 
		y));
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

void LoopAudioWindow::update()
{
	duration->update(plugin->config.duration);
}

PLUGIN_THREAD_METHODS


LoopAudioDuration::LoopAudioDuration(LoopAudio *plugin, 
	int x, 
	int y)
 : BC_TextBox(x, 
	y, 
	100,
	1,
	plugin->config.duration,
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

PLUGIN_CLASS_METHODS

AFrame *LoopAudio::process_tmpframe(AFrame *aframe)
{
	ptstime plugin_start = get_start();
	ptstime frame_pts = aframe->get_pts();
	ptstime remain;
	double period, q;
	ptstime loop_pts;

	if(load_configuration())
		update_gui();

	if(aframe->get_end_pts() - plugin_start > config.duration)
	{
		q = modf(aframe->get_pts() - plugin_start / config.duration, &period);
		loop_pts = q * config.duration + plugin_start;

		if(period >= 1.0)
		{
			// replace the whole frame
			aframe->set_fill_request(loop_pts, aframe->get_duration());
			aframe = get_frame(aframe);
			aframe->set_pts(frame_pts);
		}

		remain = (1.0 - q) * config.duration;

		if(remain < aframe->get_duration())
		{
			AFrame *input = clone_aframe(aframe);

			// replace end of frame
			input->set_fill_request(loop_pts + aframe->get_duration() -
				remain, remain);
			input = get_frame(input);
			int pos = aframe->to_samples(aframe->get_duration() * q);
			memcpy(&aframe->buffer[pos], input->buffer,
				input->get_length());
			release_aframe(input);
		}
	}
	return aframe;
}

int LoopAudio::load_configuration()
{
	KeyFrame *keyframe = get_first_keyframe();
	ptstime old_pts = config.duration;

	if(keyframe)
		read_data(keyframe);

	return need_reconfigure || !PTSEQU(old_pts, config.duration);
}

void LoopAudio::load_defaults()
{
	samplenum samples = 0;
// load the defaults
	defaults = load_defaults_file("loopaudio.rc");
	samples = defaults->get("SAMPLES", samples);
	if(samples > 0)
	{
// Check if we have reasonable samplerate
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

	output.tag.set_title("LOOPAUDIO");
	output.tag.set_property("DURATION", config.duration);
	output.append_tag();
	output.tag.set_title("/LOOPAUDIO");
	output.append_tag();
	keyframe->set_data(output.string);
}

void LoopAudio::read_data(KeyFrame *keyframe)
{
	FileXML input;
	samplenum samples = 0;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());

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
