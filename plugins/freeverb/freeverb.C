// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "aframe.h"
#include "bchash.h"
#include "bctitle.h"
#include "clip.h"
#include "filexml.h"
#include "freeverb.h"
#include "language.h"
#include "picon_png.h"
#include "pluginaclient.h"
#include "pluginwindow.h"
#include "units.h"

REGISTER_PLUGIN


FreeverbGain::FreeverbGain(FreeverbEffect *plugin, int x, int y)
 : BC_FPot(x, y, plugin->config.gain, INFINITYGAIN, 6)
{
	this->plugin = plugin;
	set_precision(.1);
}

int FreeverbGain::handle_event()
{
	plugin->config.gain = get_value();
	plugin->send_configure_change();
	return 1;
}

FreeverbRoomsize::FreeverbRoomsize(FreeverbEffect *plugin, int x, int y)
 : BC_FPot(x, y, plugin->config.roomsize, INFINITYGAIN, 0)
{
	this->plugin = plugin;
	set_precision(.01);
}

int FreeverbRoomsize::handle_event()
{
	plugin->config.roomsize = get_value();
	plugin->send_configure_change();
	return 1;
}

FreeverbDamp::FreeverbDamp(FreeverbEffect *plugin, int x, int y)
 : BC_FPot(x, y, plugin->config.damp, INFINITYGAIN, 0)
{
	this->plugin = plugin;
	set_precision(.01);
}

int FreeverbDamp::handle_event()
{
	plugin->config.damp = get_value();
	plugin->send_configure_change();
	return 1;
}

FreeverbWet::FreeverbWet(FreeverbEffect *plugin, int x, int y)
 : BC_FPot(x, y, plugin->config.wet, INFINITYGAIN, 0)
{
	this->plugin = plugin;
	set_precision(.01);
}

int FreeverbWet::handle_event()
{
	plugin->config.wet = get_value();
	plugin->send_configure_change();
	return 1;
}

FreeverbDry::FreeverbDry(FreeverbEffect *plugin, int x, int y)
 : BC_FPot(x, y, plugin->config.dry, INFINITYGAIN, 0)
{
	this->plugin = plugin;
	set_precision(.01);
}

int FreeverbDry::handle_event()
{
	plugin->config.dry = get_value();
	plugin->send_configure_change();
	return 1;
}

FreeverbWidth::FreeverbWidth(FreeverbEffect *plugin, int x, int y)
 : BC_FPot(x, y, plugin->config.width, INFINITYGAIN, 0)
{
	this->plugin = plugin;
	set_precision(.01);
}

int FreeverbWidth::handle_event()
{
	plugin->config.width = get_value();
	plugin->send_configure_change();
	return 1;
}

FreeverbMode::FreeverbMode(FreeverbEffect *plugin, int x, int y)
 : BC_CheckBox(x, y, plugin->config.mode, _("Freeze"))
{
	this->plugin = plugin;
}

int FreeverbMode::handle_event()
{
	plugin->config.mode = get_value();
	plugin->send_configure_change();
	return 1;
}


FreeverbWindow::FreeverbWindow(FreeverbEffect *plugin, int x, int y)
 : PluginWindow(plugin->gui_string, 
	x,
	y, 
	180, 
	250)
{
	int x1 = 10, x2 = 100, x3 = 135, y1 = 10, y2 = 20, margin = 30;

	add_subwindow(new BC_Title(x1, y2, _("Gain:")));
	add_subwindow(gain = new FreeverbGain(plugin, x3, y1));
	y1 += margin;
	y2 += margin;
	add_subwindow(new BC_Title(x1, y2, _("Roomsize:")));
	add_subwindow(roomsize = new FreeverbRoomsize(plugin, x2, y1));
	y1 += margin;
	y2 += margin;
	add_subwindow(new BC_Title(x1, y2, _("Damp:")));
	add_subwindow(damp = new FreeverbDamp(plugin, x3, y1));
	y1 += margin;
	y2 += margin;
	add_subwindow(new BC_Title(x1, y2, _("Wet:")));
	add_subwindow(wet = new FreeverbWet(plugin, x2, y1));
	y1 += margin;
	y2 += margin;
	add_subwindow(new BC_Title(x1, y2, _("Dry:")));
	add_subwindow(dry = new FreeverbDry(plugin, x3, y1));
	y1 += margin;
	y2 += margin;
	add_subwindow(new BC_Title(x1, y2, _("Width:")));
	add_subwindow(width = new FreeverbWidth(plugin, x2, y1));
	y1 += margin;
	y2 += margin;
	add_subwindow(mode = new FreeverbMode(plugin, x1, y2));
	PLUGIN_GUI_CONSTRUCTOR_MACRO;
}

void FreeverbWindow::update()
{
	gain->update(plugin->config.gain);
	roomsize->update(plugin->config.roomsize);
	damp->update(plugin->config.damp);
	wet->update(plugin->config.wet);
	dry->update(plugin->config.dry);
	width->update(plugin->config.width);
	mode->update((int)plugin->config.mode);
}

FreeverbConfig::FreeverbConfig()
{
	gain = -6.0;
	wet = -6.0;
	dry = 0;
	roomsize = -6.0;
	damp = 0;
	width = 0;
	mode = 0;
}

int FreeverbConfig::equivalent(FreeverbConfig &that)
{
	return EQUIV(gain, that.gain) &&
		EQUIV(wet, that.wet) &&
		EQUIV(roomsize, that.roomsize) &&
		EQUIV(dry, that.dry) &&
		EQUIV(damp, that.damp) &&
		EQUIV(width, that.width) &&
		mode == that.mode;
}

void FreeverbConfig::copy_from(FreeverbConfig &that)
{
	gain = that.gain;
	wet = that.wet;
	roomsize = that.roomsize;
	dry = that.dry;
	damp = that.damp;
	width = that.width;
	mode = that.mode;
}

void FreeverbConfig::interpolate(FreeverbConfig &prev, 
	FreeverbConfig &next, 
	ptstime prev_pts,
	ptstime next_pts,
	ptstime current_pts)
{
	PLUGIN_CONFIG_INTERPOLATE_MACRO

	gain = prev.gain * prev_scale + next.gain * next_scale;
	wet = prev.wet * prev_scale + next.wet * next_scale;
	roomsize = prev.roomsize * prev_scale + next.roomsize * next_scale;
	dry = prev.dry * prev_scale + next.dry * next_scale;
	damp = prev.damp * prev_scale + next.damp * next_scale;
	width = prev.width * prev_scale + next.width * next_scale;
	mode = prev.mode;
}

PLUGIN_THREAD_METHODS

FreeverbEffect::FreeverbEffect(PluginServer *server)
 : PluginAClient(server)
{
	engine = 0;
	temp = 0;
	temp_out = 0;
	temp_allocated = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

FreeverbEffect::~FreeverbEffect()
{
	delete engine;
	if(temp)
	{
		for(int i = 0; i < total_in_buffers; i++)
		{
			delete [] temp[i];
			delete [] temp_out[i];
		}
		delete [] temp;
		delete [] temp_out;
	}
	PLUGIN_DESTRUCTOR_MACRO
}

void FreeverbEffect::reset_plugin()
{
	if(engine)
	{
		delete engine;
		engine = 0;
	}
	if(temp)
	{
		for(int i = 0; i < total_in_buffers; i++)
		{
			delete [] temp[i];
			delete [] temp_out[i];
		}
		delete [] temp;
		temp = 0;
		delete [] temp_out;
		temp_out = 0;
		temp_allocated = 0;
	}
}

PLUGIN_CLASS_METHODS

void FreeverbEffect::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());

	int result = 0;
	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("FREEVERB"))
			{
				config.gain = input.tag.get_property("GAIN", config.gain);
				config.roomsize = input.tag.get_property("ROOMSIZE", config.roomsize);
				config.damp = input.tag.get_property("DAMP", config.damp);
				config.wet = input.tag.get_property("WET", config.wet);
				config.dry = input.tag.get_property("DRY", config.dry);
				config.width = input.tag.get_property("WIDTH", config.width);
				config.mode = input.tag.get_property("MODE", config.mode);
			}
		}
	}
}

void FreeverbEffect::save_data(KeyFrame *keyframe)
{
	FileXML output;

	output.tag.set_title("FREEVERB");
	output.tag.set_property("GAIN", config.gain);
	output.tag.set_property("ROOMSIZE", config.roomsize);
	output.tag.set_property("DAMP", config.damp);
	output.tag.set_property("WET", config.wet);
	output.tag.set_property("DRY", config.dry);
	output.tag.set_property("WIDTH", config.width);
	output.tag.set_property("MODE", config.mode);
	output.append_tag();
	output.tag.set_title("/FREEVERB");
	output.append_tag();
	keyframe->set_data(output.string);
}

void FreeverbEffect::load_defaults()
{
	defaults = load_defaults_file("freeverb.rc");

	config.gain = defaults->get("GAIN", config.gain);
	config.roomsize = defaults->get("ROOMSIZE", config.roomsize);
	config.damp = defaults->get("DAMP", config.damp);
	config.wet = defaults->get("WET", config.wet);
	config.dry = defaults->get("DRY", config.dry);
	config.width = defaults->get("WIDTH", config.width);
	config.mode = defaults->get("MODE", config.mode);
}

void FreeverbEffect::save_defaults()
{
	defaults->update("GAIN", config.gain);
	defaults->update("ROOMSIZE", config.roomsize);
	defaults->update("DAMP", config.damp);
	defaults->update("WET", config.wet);
	defaults->update("DRY", config.dry);
	defaults->update("WIDTH", config.width);
	defaults->update("MODE", config.mode);
	defaults->save();
}

void FreeverbEffect::process_tmpframes(AFrame **input)
{
	int size = input[0]->get_length();

	if(load_configuration())
		update_gui();

	if(!engine)
		engine = new revmodel;

	engine->setroomsize(DB::fromdb(config.roomsize));
	engine->setdamp(DB::fromdb(config.damp));
	engine->setwet(DB::fromdb(config.wet));
	engine->setdry(DB::fromdb(config.dry));
	engine->setwidth(DB::fromdb(config.width));
	engine->setmode(config.mode);

	float gain_f = DB::fromdb(config.gain);

	if(size > temp_allocated)
	{
		if(temp)
		{
			for(int i = 0; i < total_in_buffers; i++)
			{
				delete [] temp[i];
				delete [] temp_out[i];
			}
			delete [] temp;
			delete [] temp_out;
		}
		temp = 0;
		temp_out = 0;
	}
	if(!temp)
	{
		temp_allocated = size * 2;
		temp = new float*[total_in_buffers];
		temp_out = new float*[total_in_buffers];
		for(int i = 0; i < total_in_buffers; i++)
		{
			temp[i] = new float[temp_allocated];
			temp_out[i] = new float[temp_allocated];
		}
	}

	for(int i = 0; i < 2 && i < total_in_buffers; i++)
	{
		float *out = temp[i];
		double *in = input[i]->buffer;
		for(int j = 0; j < size; j++)
		{
			out[j] = in[j];
		}
	}

	if(total_in_buffers < 2)
	{
		engine->processreplace(temp[0], 
			temp[0], 
			temp_out[0], 
			temp_out[0], 
			size, 
			1);
	}
	else
	{
		engine->processreplace(temp[0], 
			temp[1], 
			temp_out[0], 
			temp_out[1], 
			size, 
			1);
	}

	for(int i = 0; i < 2 && i < total_in_buffers; i++)
	{
		double *out = input[i]->buffer;
		float *in = temp_out[i];
		for(int j = 0; j < size; j++)
		{
			out[j] = gain_f * in[j];
		}
	}
}
