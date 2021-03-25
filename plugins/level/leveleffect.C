// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "aframe.h"
#include "clip.h"
#include "bchash.h"
#include "bctitle.h"
#include "filexml.h"
#include "language.h"
#include "leveleffect.h"
#include "picon_png.h"
#include "units.h"


REGISTER_PLUGIN

SoundLevelConfig::SoundLevelConfig()
{
	duration = 1.0;
}

void SoundLevelConfig::copy_from(SoundLevelConfig &that)
{
	duration = that.duration;
}

int SoundLevelConfig::equivalent(SoundLevelConfig &that)
{
	return EQUIV(duration, that.duration);
}

void SoundLevelConfig::interpolate(SoundLevelConfig &prev, 
	SoundLevelConfig &next, 
	ptstime prev_pts,
	ptstime next_pts,
	ptstime current_pts)
{
	duration = prev.duration;
}


SoundLevelDuration::SoundLevelDuration(SoundLevelEffect *plugin, int x, int y)
 : BC_FSlider(x, y, 0, 180, 180, 0.0, 10.0, plugin->config.duration)
{
	this->plugin = plugin;
	set_precision(0.1);
}

int SoundLevelDuration::handle_event()
{
	plugin->config.duration = get_value();
	plugin->send_configure_change();
	return 1;
}


SoundLevelWindow::SoundLevelWindow(SoundLevelEffect *plugin, int x, int y)
 : PluginWindow(plugin,
	x,
	y, 
	350, 
	120)
{
	x = y = 10;

	add_subwindow(new BC_Title(x, y, _("Duration (seconds):")));
	add_subwindow(duration = new SoundLevelDuration(plugin, x + 150, y));
	y += 35;
	add_subwindow(new BC_Title(x, y, _("Max soundlevel (dB):")));
	add_subwindow(soundlevel_max = new BC_Title(x + 150, y, (const char*)0));
	y += 35;
	add_subwindow(new BC_Title(x, y, _("RMS soundlevel (dB):")));
	add_subwindow(soundlevel_rms = new BC_Title(x + 150, y, (const char*)0));
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

void SoundLevelWindow::update()
{
	duration->update(plugin->config.duration);
}

PLUGIN_THREAD_METHODS

SoundLevelEffect::SoundLevelEffect(PluginServer *server)
 : PluginAClient(server)
{
	rms_accum = 0;
	max_accum = 0;
	accum_size = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

SoundLevelEffect::~SoundLevelEffect()
{
	PLUGIN_DESTRUCTOR_MACRO
}

PLUGIN_CLASS_METHODS


void SoundLevelEffect::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());

	int result = 0;
	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("SOUNDLEVEL"))
			{
				config.duration = input.tag.get_property("DURATION", config.duration);
			}
		}
	}
}

void SoundLevelEffect::save_data(KeyFrame *keyframe)
{
	FileXML output;

	output.tag.set_title("SOUNDLEVEL");
	output.tag.set_property("DURATION", config.duration);
	output.append_tag();
	output.tag.set_title("/SOUNDLEVEL");
	output.append_tag();
	keyframe->set_data(output.string);
}

void SoundLevelEffect::load_defaults()
{
	defaults = load_defaults_file("soundlevel.rc");
	config.duration = defaults->get("DURATION", config.duration);
}

void SoundLevelEffect::save_defaults()
{
	defaults->update("DURATION", config.duration);
	defaults->save();
}

AFrame *SoundLevelEffect::process_tmpframe(AFrame *input)
{
	int size = input->get_length();

	if(load_configuration())
		update_gui();

	accum_size += size;
	for(int i = 0; i < size; i++)
	{
		double value = fabs(input->buffer[i]);
		if(value > max_accum) max_accum = value;
		rms_accum += value * value;
	}

	if(accum_size > input->to_samples(config.duration))
	{
		rms_accum = sqrt(rms_accum / accum_size);
		render_gui();
		rms_accum = 0;
		max_accum = 0;
		accum_size = 0;
	}
	return input;
}

void SoundLevelEffect::render_gui()
{
	if(thread)
	{
		thread->window->soundlevel_max->update(DB::todb(max_accum));
		thread->window->soundlevel_rms->update(DB::todb(rms_accum));
		thread->window->flush();
	}
}
