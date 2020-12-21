// SPDX-License-Identifier: GPL-2.0-or-later

// This file is part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "atmpframecache.h"
#include "bchash.h"
#include "denoisefft.h"
#include "clip.h"
#include "filexml.h"
#include "fourier.h"
#include "picon_png.h"
#include "plugin.h"
#include "units.h"

#include <math.h>
#include <string.h>

#define MIN_REFERENCE 0.25
#define MAX_REFERENCE 10.0

REGISTER_PLUGIN

DenoiseFFTConfig::DenoiseFFTConfig()
{
	referencetime = MIN_REFERENCE;
	level = 0.0;
}


DenoiseFFTLevel::DenoiseFFTLevel(DenoiseFFTEffect *plugin, int x, int y)
 : BC_FPot(x, y, plugin->config.level, INFINITYGAIN, 6.0)
{
	this->plugin = plugin;
	set_precision(0.1);
}

int DenoiseFFTLevel::handle_event()
{
	plugin->config.level = get_value();
	plugin->send_configure_change();
	return 1;
}

DenoiseFFTSamples::DenoiseFFTSamples(DenoiseFFTEffect *plugin, 
	int x, 
	int y, 
	char *text)
 : BC_PopupMenu(x, y, 100, text, 1)
{
	this->plugin = plugin;
}

int DenoiseFFTSamples::handle_event()
{
	plugin->config.referencetime = atof(get_text());
	plugin->send_configure_change();
	return 1;
}


DenoiseFFTWindow::DenoiseFFTWindow(DenoiseFFTEffect *plugin, int x, int y)
 : PluginWindow(plugin->gui_string, 
	x,
	y, 
	300, 
	130)
{
	x = y = 10;

	add_subwindow(new BC_Title(x, y, _("Denoise power:")));
	add_subwindow(level = new DenoiseFFTLevel(plugin, x + 130, y));
	y += level->get_h() + 10;
	add_subwindow(new BC_Title(x, y, _("Reference duration:")));
	y += 20;
	add_subwindow(new BC_Title(x, y, _("The keyframe is the start of the reference")));
	y += 20;

	char string[BCTEXTLEN];
	sprintf(string, "%.2f", plugin->config.referencetime);
	add_subwindow(samples = new DenoiseFFTSamples(plugin, x + 100, y, string));
	for(double f = MIN_REFERENCE; f < MAX_REFERENCE; f *= 2)
	{
		sprintf(string, "%.2f", f);
		samples->add_item(new BC_MenuItem(string));
	}
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

void DenoiseFFTWindow::update()
{
	char string[BCTEXTLEN];

	level->update(plugin->config.level);
	sprintf(string, "%.2f", plugin->config.referencetime);
	samples->set_text(string);
}

PLUGIN_THREAD_METHODS

DenoiseFFTEffect::DenoiseFFTEffect(PluginServer *server)
 : PluginAClient(server)
{
	reference = 0;
	remove_engine = 0;
	collect_engine = 0;
	collection_pts = -1;
	PLUGIN_CONSTRUCTOR_MACRO
}

DenoiseFFTEffect::~DenoiseFFTEffect()
{
	PLUGIN_DESTRUCTOR_MACRO
	delete [] reference;
	delete remove_engine;
	delete collect_engine;
}

void DenoiseFFTEffect::reset_plugin()
{
	delete [] reference;
	reference = 0;
	delete remove_engine;
	remove_engine = 0;
	delete collect_engine;
	collect_engine = 0;
	collection_pts = -1;
}

PLUGIN_CLASS_METHODS

void DenoiseFFTEffect::read_data(KeyFrame *keyframe)
{
	FileXML input;
	samplenum samples = 0;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());

	int result = 0;
	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("DENOISEFFT"))
			{
				samples = input.tag.get_property("SAMPLES", samples);
				if(samples > 0)
					config.referencetime = (ptstime)samples / get_project_samplerate();
				config.referencetime = input.tag.get_property(
					"REFERENCETIME", config.referencetime);
				config.level = input.tag.get_property("LEVEL", config.level);
			}
		}
	}
}

void DenoiseFFTEffect::save_data(KeyFrame *keyframe)
{
	FileXML output;

	output.tag.set_title("DENOISEFFT");
	output.tag.set_property("REFERENCETIME", config.referencetime);
	output.tag.set_property("LEVEL", config.level);
	output.append_tag();
	output.tag.set_title("/DENOISEFFT");
	output.append_tag();
	keyframe->set_data(output.string);
}

void DenoiseFFTEffect::load_defaults()
{
	samplenum samples = 0;
	defaults = load_defaults_file("denoisefft.rc");

	config.level = defaults->get("LEVEL", config.level);
	samples = defaults->get("SAMPLES", samples);
	if(samples > 0)
		config.referencetime = (ptstime)samples / get_project_samplerate();
	config.referencetime = defaults->get("REFERENCETIME", config.referencetime);
}

void DenoiseFFTEffect::save_defaults()
{
	defaults->update("LEVEL", config.level);
	defaults->delete_key("SAMPLES");
	defaults->update("REFERENCETIME", config.referencetime);
	defaults->save();
}

int DenoiseFFTEffect::load_configuration()
{
	KeyFrame *prev_keyframe = get_prev_keyframe(source_pts);

	if(!prev_keyframe)
		return get_need_reconfigure();

	ptstime prev_pts = prev_keyframe->pos_time;

	read_data(prev_keyframe);
	if(prev_pts <= 0)
		prev_pts = get_start();

	if(!PTSEQU(prev_pts, collection_pts))
	{
		collection_pts = prev_pts;
		return 1;
	}
	return get_need_reconfigure();
}

AFrame *DenoiseFFTEffect::process_tmpframe(AFrame *aframe)
{
	input_frame = aframe;

// Do noise collection
	if(load_configuration())
	{
		update_gui();
		collect_noise();
	}

// Remove noise
	if(!remove_engine)
		remove_engine = new DenoiseFFTRemove(this, aframe->get_buffer_length());

	aframe = remove_engine->process_frame(aframe);
	return aframe;
}

void DenoiseFFTEffect::collect_noise()
{
	AFrame *noise_frame;
	int window_size = input_frame->get_buffer_length();
	int half_window = window_size / 2;

	if(!reference)
		reference = new double[half_window];
	memset(reference, 0, sizeof(double) * half_window);

	if(!collect_engine)
		collect_engine = new DenoiseFFTCollect(this, window_size);

	noise_frame = audio_frames.clone_frame(input_frame);

	int total_windows = 0;
	ptstime collection_end = collection_pts + config.referencetime;

	if(collection_end > plugin->end_pts())
		collection_end = plugin->end_pts();

	for(ptstime t = collection_pts; t < collection_end;
			t += noise_frame->get_duration())
	{
		noise_frame->set_fill_request(t, window_size);
		noise_frame = get_frame(noise_frame);
		noise_frame = collect_engine->process_frame(noise_frame);
		total_windows++;
	}
	audio_frames.release_frame(noise_frame);

	for(int i = 0; i < half_window / 2; i++)
		reference[i] /= total_windows;
}

DenoiseFFTRemove::DenoiseFFTRemove(DenoiseFFTEffect *plugin, int window_size)
 : Fourier(window_size)
{
	this->plugin = plugin;
}

int DenoiseFFTRemove::signal_process()
{
	int half_window = get_window_size() / 2;
	double level = DB::fromdb(plugin->config.level);

	for(int i = 1; i < half_window; i++)
	{
		double re = fftw_window[i][0];
		double im = fftw_window[i][1];
		double result = sqrt(re * re + im * im);
		double angle = atan2(im, re);
		result -= plugin->reference[i] * level;
		if(result < 0)
			result = 0;
		fftw_window[i][0] = result * cos(angle);
		fftw_window[i][1] = result * sin(angle);
	}
	return 1;
}

DenoiseFFTCollect::DenoiseFFTCollect(DenoiseFFTEffect *plugin, int window_size)
 : Fourier(window_size)
{
	this->plugin = plugin;
}

int DenoiseFFTCollect::signal_process()
{
	int half_window = get_window_size() / 2;

	plugin->reference[0] = 0;
	for(int i = 1; i < half_window; i++)
	{
		double re = fftw_window[i][0];
		double im = fftw_window[i][1];

		plugin->reference[i] += sqrt(re * re + im * im);
	}
	return 0;
}
