
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

#define PLUGIN_TITLE N_("DenoiseFFT")
#define PLUGIN_IS_AUDIO
#define PLUGIN_IS_REALTIME
#define PLUGIN_CUSTOM_LOAD_CONFIGURATION
#define PLUGIN_CLASS DenoiseFFTEffect
#define PLUGIN_CONFIG_CLASS DenoiseFFTConfig
#define PLUGIN_THREAD_CLASS DenoiseFFTThread
#define PLUGIN_GUI_CLASS DenoiseFFTWindow

#include "pluginmacros.h"
#include "clip.h"
#include "bchash.h"
#include "filexml.h"
#include "guicast.h"
#include "language.h"
#include "fourier.h"
#include "picon_png.h"
#include "pluginaclient.h"
#include "units.h"
#include "vframe.h"

#include <errno.h>
#include <math.h>
#include <string.h>

#define WINDOW_SIZE 16384

// Noise collection is done either from the start of the effect or the start
// of the previous keyframe.  It always covers the higher numbered samples
// after the keyframe.

class DenoiseFFTConfig
{
public:
	DenoiseFFTConfig();

	int samples;
	double level;
	PLUGIN_CONFIG_CLASS_MEMBERS
};

class DenoiseFFTLevel : public BC_FPot
{
public:
	DenoiseFFTLevel(DenoiseFFTEffect *plugin, int x, int y);
	int handle_event();
	DenoiseFFTEffect *plugin;
};

class DenoiseFFTSamples : public BC_PopupMenu
{
public:
	DenoiseFFTSamples(DenoiseFFTEffect *plugin, int x, int y, char *text);
	int handle_event();
	DenoiseFFTEffect *plugin;
};

class DenoiseFFTWindow : public BC_Window
{
public:
	DenoiseFFTWindow(DenoiseFFTEffect *plugin, int x, int y);
	~DenoiseFFTWindow();

	void update();

	DenoiseFFTLevel *level;
	DenoiseFFTSamples *samples;
	PLUGIN_GUI_CLASS_MEMBERS
};


PLUGIN_THREAD_HEADER


class DenoiseFFTRemove : public CrossfadeFFT
{
public:
	DenoiseFFTRemove(DenoiseFFTEffect *plugin, int window_size);
	void signal_process();
	void get_frame(AFrame *aframe);
	DenoiseFFTEffect *plugin;
};

class DenoiseFFTCollect : public CrossfadeFFT
{
public:
	DenoiseFFTCollect(DenoiseFFTEffect *plugin, int window_size);
	void signal_process();
	void get_frame(AFrame *aframe);
	DenoiseFFTEffect *plugin;
};

class DenoiseFFTEffect : public PluginAClient
{
public:
	DenoiseFFTEffect(PluginServer *server);
	~DenoiseFFTEffect();

	void read_data(KeyFrame *keyframe);
	void save_data(KeyFrame *keyframe);
	void process_frame(AFrame *aframe);
	void collect_noise();

	void load_defaults();
	void save_defaults();

	void process_window();

	PLUGIN_CLASS_MEMBERS

// Need to sample noise now.
	int need_collection;
	AFrame noise_frame;
// Start of sample of noise to collect
	ptstime collection_pts;
	double *reference;
	DenoiseFFTRemove *remove_engine;
	DenoiseFFTCollect *collect_engine;
};


REGISTER_PLUGIN


DenoiseFFTConfig::DenoiseFFTConfig()
{
	samples = WINDOW_SIZE;
	level = 0.0;
}


DenoiseFFTLevel::DenoiseFFTLevel(DenoiseFFTEffect *plugin, int x, int y)
 : BC_FPot(x, y, (float)plugin->config.level, INFINITYGAIN, 6.0)
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
	plugin->config.samples = atol(get_text());
	plugin->send_configure_change();
	return 1;
}


DenoiseFFTWindow::DenoiseFFTWindow(DenoiseFFTEffect *plugin, int x, int y)
 : BC_Window(plugin->gui_string, 
	x,
	y, 
	300, 
	130, 
	300, 
	130,
	0, 
	0,
	1)
{
	x = y = 10;

	add_subwindow(new BC_Title(x, y, _("Denoise power:")));
	add_subwindow(level = new DenoiseFFTLevel(plugin, x + 130, y));
	y += level->get_h() + 10;
	add_subwindow(new BC_Title(x, y, _("Number of samples for reference:")));
	y += 20;
	add_subwindow(new BC_Title(x, y, _("The keyframe is the start of the reference")));
	y += 20;

	char string[BCTEXTLEN];
	sprintf(string, "%d\n", plugin->config.samples);
	add_subwindow(samples = new DenoiseFFTSamples(plugin, x + 100, y, string));
	for(int i = WINDOW_SIZE; i < 0x100000; )
	{
		sprintf(string, "%d", i);
		samples->add_item(new BC_MenuItem(string));
		i *= 2;
	}
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

DenoiseFFTWindow::~DenoiseFFTWindow()
{
}

void DenoiseFFTWindow::update()
{
	char string[BCTEXTLEN];

	level->update(plugin->config.level);
	sprintf(string, "%d", plugin->config.samples);
	samples->set_text(string);
}

PLUGIN_THREAD_METHODS

DenoiseFFTEffect::DenoiseFFTEffect(PluginServer *server)
 : PluginAClient(server)
{
	reference = 0;
	remove_engine = 0;
	collect_engine = 0;
	need_collection = 1;
	collection_pts = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

DenoiseFFTEffect::~DenoiseFFTEffect()
{
	PLUGIN_DESTRUCTOR_MACRO
	if(reference) delete [] reference;
	if(remove_engine) delete remove_engine;
	if(collect_engine) delete collect_engine;
}

PLUGIN_CLASS_METHODS

void DenoiseFFTEffect::read_data(KeyFrame *keyframe)
{
	FileXML input;
	input.set_shared_string(keyframe->data, strlen(keyframe->data));

	int result = 0;
	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("DENOISEFFT"))
			{
				config.samples = input.tag.get_property("SAMPLES", config.samples);
				config.level = input.tag.get_property("LEVEL", config.level);
			}
		}
	}
}

void DenoiseFFTEffect::save_data(KeyFrame *keyframe)
{
	FileXML output;
	output.set_shared_string(keyframe->data, MESSAGESIZE);

	output.tag.set_title("DENOISEFFT");
	output.tag.set_property("SAMPLES", config.samples);
	output.tag.set_property("LEVEL", config.level);
	output.append_tag();
	output.tag.set_title("/DENOISEFFT");
	output.append_tag();
	output.append_newline();

	output.terminate_string();
}

void DenoiseFFTEffect::load_defaults()
{
	defaults = load_defaults_file("denoisefft.rc");

	config.level = defaults->get("LEVEL", config.level);
	config.samples = defaults->get("SAMPLES", config.samples);
}

void DenoiseFFTEffect::save_defaults()
{
	defaults->update("LEVEL", config.level);
	defaults->update("SAMPLES", config.samples);
	defaults->save();
}

int DenoiseFFTEffect::load_configuration()
{
	KeyFrame *prev_keyframe = prev_keyframe_pts(source_pts);
	ptstime prev_pts = prev_keyframe->pos_time;

	read_data(prev_keyframe);
	if(prev_pts <= 0) prev_pts = source_start_pts;

	if(!PTSEQU(prev_pts, collection_pts))
	{
		collection_pts = prev_pts;
		need_collection = 1;
	}
	return 0;
}

void DenoiseFFTEffect::process_frame(AFrame *aframe)
{
	load_configuration();
// Do noise collection
	if(need_collection)
	{
		need_collection = 0;
		noise_frame.samplerate = aframe->samplerate;
		collect_noise();
	}

// Remove noise
	if(!remove_engine)
		remove_engine = new DenoiseFFTRemove(this, WINDOW_SIZE);

	remove_engine->process_frame(aframe);
}

void DenoiseFFTEffect::collect_noise()
{
	if(!reference) reference = new double[WINDOW_SIZE / 2];
	if(!collect_engine)
	{
		collect_engine = new DenoiseFFTCollect(this, WINDOW_SIZE);
	}
	memset(reference, 0, sizeof(double) * WINDOW_SIZE / 2);

	samplenum collection_start = noise_frame.to_samples(collection_pts);
	int total_windows = 0;

	for(int i = 0; i < config.samples; i += WINDOW_SIZE)
	{
// AFrame without buffer
		noise_frame.set_fill_request(collection_start, WINDOW_SIZE);
		collect_engine->process_frame(&noise_frame);
		collection_start += WINDOW_SIZE;
		total_windows++;
	}

	for(int i = 0; i < WINDOW_SIZE / 2; i++)
	{
		reference[i] /= total_windows;
	}
}

DenoiseFFTRemove::DenoiseFFTRemove(DenoiseFFTEffect *plugin, int window_size)
 : CrossfadeFFT(window_size)
{
	this->plugin = plugin;
}

void DenoiseFFTRemove::signal_process()
{
	double level = DB::fromdb(plugin->config.level);
	for(int i = 0; i < window_size / 2; i++)
	{
		double result = sqrt(freq_real[i] * freq_real[i] + freq_imag[i] * freq_imag[i]);
		double angle = atan2(freq_imag[i], freq_real[i]);
		result -= plugin->reference[i] * level;
		if(result < 0) result = 0;
		freq_real[i] = result * cos(angle);
		freq_imag[i] = result * sin(angle);
	}
	symmetry(window_size, freq_real, freq_imag);
}

void DenoiseFFTRemove::get_frame(AFrame *aframe)
{
	plugin->get_frame(aframe);
}


DenoiseFFTCollect::DenoiseFFTCollect(DenoiseFFTEffect *plugin, int window_size)
 : CrossfadeFFT(window_size)
{
	this->plugin = plugin;
}

void DenoiseFFTCollect::signal_process()
{
	for(int i = 0; i < window_size / 2; i++)
	{
		double result = sqrt(freq_real[i] * freq_real[i] + freq_imag[i] * freq_imag[i]);
		plugin->reference[i] += result;
	}
}

void DenoiseFFTCollect::get_frame(AFrame *aframe)
{
	plugin->get_frame(aframe);
}
