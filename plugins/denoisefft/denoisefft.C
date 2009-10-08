
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

#include "bcdisplayinfo.h"
#include "clip.h"
#include "bchash.h"
#include "filesystem.h"
#include "filexml.h"
#include "guicast.h"
#include "language.h"
#include "mutex.h"
#include "fourier.h"
#include "picon_png.h"
#include "pluginaclient.h"
#include "transportque.inc"
#include "units.h"
#include "vframe.h"

#include <errno.h>
#include <math.h>
#include <string.h>



#define WINDOW_SIZE 16384


// Noise collection is done either from the start of the effect or the start
// of the previous keyframe.  It always covers the higher numbered samples
// after the keyframe.

class DenoiseFFTEffect;
class DenoiseFFTWindow;




class DenoiseFFTConfig
{
public:
	DenoiseFFTConfig();

	int samples;
	double level;
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
	void create_objects();
	int close_event();
	DenoiseFFTLevel *level;
	DenoiseFFTSamples *samples;
	DenoiseFFTEffect *plugin;
};













PLUGIN_THREAD_HEADER(DenoiseFFTEffect, DenoiseFFTThread, DenoiseFFTWindow)


class DenoiseFFTRemove : public CrossfadeFFT
{
public:
	DenoiseFFTRemove(DenoiseFFTEffect *plugin);
	int signal_process();
	int read_samples(int64_t output_sample, 
		int samples, 
		double *buffer);
	DenoiseFFTEffect *plugin;
};

class DenoiseFFTCollect : public CrossfadeFFT
{
public:
	DenoiseFFTCollect(DenoiseFFTEffect *plugin);
	int signal_process();
	int read_samples(int64_t output_sample, 
		int samples, 
		double *buffer);
	DenoiseFFTEffect *plugin;
};

class DenoiseFFTEffect : public PluginAClient
{
public:
	DenoiseFFTEffect(PluginServer *server);
	~DenoiseFFTEffect();

	int is_realtime();
	void read_data(KeyFrame *keyframe);
	void save_data(KeyFrame *keyframe);
	int process_buffer(int64_t size, 
		double *buffer,
		int64_t start_position,
		int sample_rate);
	void collect_noise();



	int load_defaults();
	int save_defaults();
	void reset();
	void update_gui();

	void process_window();


	PLUGIN_CLASS_MEMBERS(DenoiseFFTConfig, DenoiseFFTThread)

// Need to sample noise now.
	int need_collection;
// Start of sample of noise to collect
	int64_t collection_sample;
	double *reference;
	DenoiseFFTRemove *remove_engine;
	DenoiseFFTCollect *collect_engine;
};










REGISTER_PLUGIN(DenoiseFFTEffect)









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
	this->plugin = plugin;
}

void DenoiseFFTWindow::create_objects()
{
	int x = 10, y = 10;

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
	show_window();
	flush();
}

WINDOW_CLOSE_EVENT(DenoiseFFTWindow)











PLUGIN_THREAD_OBJECT(DenoiseFFTEffect, DenoiseFFTThread, DenoiseFFTWindow)









DenoiseFFTEffect::DenoiseFFTEffect(PluginServer *server)
 : PluginAClient(server)
{
	reset();
	PLUGIN_CONSTRUCTOR_MACRO
}

DenoiseFFTEffect::~DenoiseFFTEffect()
{
	PLUGIN_DESTRUCTOR_MACRO
	if(reference) delete [] reference;
	if(remove_engine) delete remove_engine;
	if(collect_engine) delete collect_engine;
}

NEW_PICON_MACRO(DenoiseFFTEffect)

SHOW_GUI_MACRO(DenoiseFFTEffect, DenoiseFFTThread)

RAISE_WINDOW_MACRO(DenoiseFFTEffect)

SET_STRING_MACRO(DenoiseFFTEffect)


void DenoiseFFTEffect::reset()
{
	reference = 0;
	remove_engine = 0;
	collect_engine = 0;
	need_collection = 1;
	collection_sample = 0;
}

int DenoiseFFTEffect::is_realtime() { return 1; }
char* DenoiseFFTEffect::plugin_title() { return N_("DenoiseFFT"); }





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

int DenoiseFFTEffect::load_defaults()
{
	defaults = new BC_Hash(BCASTDIR "denoisefft.rc");
	defaults->load();

	config.level = defaults->get("LEVEL", config.level);
	config.samples = defaults->get("SAMPLES", config.samples);
	return 0;
}

int DenoiseFFTEffect::save_defaults()
{
	char string[BCTEXTLEN];

	defaults->update("LEVEL", config.level);
	defaults->update("SAMPLES", config.samples);
	defaults->save();

	return 0;
}

void DenoiseFFTEffect::update_gui()
{
	if(thread)
	{
		load_configuration();
		thread->window->lock_window();
		thread->window->level->update(config.level);
		char string[BCTEXTLEN];
		sprintf(string, "%d", config.samples);
		thread->window->samples->set_text(string);
		thread->window->unlock_window();
	}
}

int DenoiseFFTEffect::load_configuration()
{
	KeyFrame *prev_keyframe = get_prev_keyframe(get_source_position());
	int64_t prev_position = edl_to_local(prev_keyframe->position);
	read_data(prev_keyframe);
	if(prev_position == 0) prev_position = get_source_start();

	if(prev_position != collection_sample)
	{
		collection_sample = prev_position;
		need_collection = 1;
	}
	return 0;
}

int DenoiseFFTEffect::process_buffer(int64_t size, 
		double *buffer,
		int64_t start_position,
		int sample_rate)
{
	load_configuration();

// Do noise collection
	if(need_collection)
	{
		need_collection = 0;
		collect_noise();
	}

// Remove noise
	if(!remove_engine)
	{
		remove_engine = new DenoiseFFTRemove(this);
		remove_engine->initialize(WINDOW_SIZE);
	}
	remove_engine->process_buffer(start_position, 
		size,
		buffer, 
		get_direction());

	return 0;
}


void DenoiseFFTEffect::collect_noise()
{
	if(!reference) reference = new double[WINDOW_SIZE / 2];
	if(!collect_engine)
	{
		collect_engine = new DenoiseFFTCollect(this);
		collect_engine->initialize(WINDOW_SIZE);
	}
	bzero(reference, sizeof(double) * WINDOW_SIZE / 2);

	int64_t collection_start = collection_sample;
	int step = 1;
	int total_windows = 0;

	if(get_direction() == PLAY_REVERSE)
	{
		collection_start += config.samples;
		step = -1;
	}

	for(int i = 0; i < config.samples; i += WINDOW_SIZE)
	{
		collect_engine->process_buffer(collection_start,
			WINDOW_SIZE,
			0,
			get_direction());

		collection_start += step * WINDOW_SIZE;
		total_windows++;
	}

	for(int i = 0; i < WINDOW_SIZE / 2; i++)
	{
		reference[i] /= total_windows;
	}
}








DenoiseFFTRemove::DenoiseFFTRemove(DenoiseFFTEffect *plugin)
{
	this->plugin = plugin;
}

int DenoiseFFTRemove::signal_process()
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
	return 0;
}

int DenoiseFFTRemove::read_samples(int64_t output_sample, 
	int samples, 
	double *buffer)
{
	return plugin->read_samples(buffer,
		0,
		plugin->get_samplerate(),
		output_sample,
		samples);
}





DenoiseFFTCollect::DenoiseFFTCollect(DenoiseFFTEffect *plugin)
{
	this->plugin = plugin;
}

int DenoiseFFTCollect::signal_process()
{
	for(int i = 0; i < window_size / 2; i++)
	{
		double result = sqrt(freq_real[i] * freq_real[i] + freq_imag[i] * freq_imag[i]);
		plugin->reference[i] += result;
	}
	return 0;
}

int DenoiseFFTCollect::read_samples(int64_t output_sample, 
	int samples, 
	double *buffer)
{
	return plugin->read_samples(buffer,
		0,
		plugin->get_samplerate(),
		output_sample,
		samples);
}

