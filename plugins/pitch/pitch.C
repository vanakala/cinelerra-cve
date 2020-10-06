// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bctitle.h"
#include "clip.h"
#include "bchash.h"
#include "filexml.h"
#include "language.h"
#include "pitch.h"
#include "picon_png.h"
#include "units.h"

#define WINDOW_SIZE 2048

REGISTER_PLUGIN


PitchEffect::PitchEffect(PluginServer *server)
 : PluginAClient(server)
{
	PLUGIN_CONSTRUCTOR_MACRO
	fft = 0;
}

PitchEffect::~PitchEffect()
{
	PLUGIN_DESTRUCTOR_MACRO
	delete fft;
}

void PitchEffect::reset_plugin()
{
	if(fft)
	{
		delete fft;
		fft = 0;
	}
}

PLUGIN_CLASS_METHODS

void PitchEffect::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());

	int result = 0;
	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("PITCH"))
			{
				config.scale = input.tag.get_property("SCALE", config.scale);
			}
		}
	}
}

void PitchEffect::save_data(KeyFrame *keyframe)
{
	FileXML output;

	output.tag.set_title("PITCH");
	output.tag.set_property("SCALE", config.scale);
	output.append_tag();
	output.tag.set_title("/PITCH");
	output.append_tag();
	keyframe->set_data(output.string);
}

void PitchEffect::load_defaults()
{
	defaults = load_defaults_file("pitch.rc");

	config.scale = defaults->get("SCALE", config.scale);
}

void PitchEffect::save_defaults()
{
	defaults->update("SCALE", config.scale);
	defaults->save();
}

AFrame *PitchEffect::process_tmpframe(AFrame *aframe)
{
	load_configuration();

	if(!fft)
	{
		fft = new Pitch(aframe->get_samplerate(), WINDOW_SIZE);
		fft->set_scale(config.scale);
	}
	else if(need_reconfigure)
		fft->set_scale(config.scale);

	aframe = fft->process_frame(aframe);
	return aframe;
}

PitchConfig::PitchConfig()
{
	scale = 1.0;
}

int PitchConfig::equivalent(PitchConfig &that)
{
	return EQUIV(scale, that.scale);
}

void PitchConfig::copy_from(PitchConfig &that)
{
	scale = that.scale;
}

void PitchConfig::interpolate(PitchConfig &prev, 
	PitchConfig &next, 
	ptstime prev_pts,
	ptstime next_pts,
	ptstime current_pts)
{
	PLUGIN_CONFIG_INTERPOLATE_MACRO
	scale = prev.scale * prev_scale + next.scale * next_scale;
}


PLUGIN_THREAD_METHODS

PitchWindow::PitchWindow(PitchEffect *plugin, int x, int y)
 : PluginWindow(plugin->gui_string, 
	x, 
	y, 
	150, 
	50)
{
	x = y = 10;

	add_subwindow(new BC_Title(x, y, _("Scale:")));
	x += 70;
	add_subwindow(scale = new PitchScale(plugin, x, y));
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

void PitchWindow::update()
{
	scale->update(plugin->config.scale);
}


PitchScale::PitchScale(PitchEffect *plugin, int x, int y)
 : BC_FPot(x, y, plugin->config.scale, .5, 1.5)
{
	this->plugin = plugin;
	set_precision(0.01);
}

int PitchScale::handle_event()
{
	plugin->config.scale = get_value();
	plugin->send_configure_change();
	return 1;
}
