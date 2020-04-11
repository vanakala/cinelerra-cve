
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

#include "aframe.h"
#include "clip.h"
#include "bchash.h"
#include "mainerror.h"
#include "filexml.h"
#include "language.h"
#include "picon_png.h"
#include "reverb.h"
#include "reverbwindow.h"

#include <string.h>

REGISTER_PLUGIN


Reverb::Reverb(PluginServer *server)
 : PluginAClient(server)
{
	srand(time(0));
	redo_buffers = 1;       // set to redo buffers before the first render
	dsp_in_length = 0;
	ref_channels = 0;
	ref_offsets = 0;
	ref_levels = 0;
	ref_lowpass = 0;
	dsp_in = 0;
	lowpass_in1 = 0;
	lowpass_in2 = 0;
	initialized = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

Reverb::~Reverb()
{
	PLUGIN_DESTRUCTOR_MACRO

	if(initialized)
	{
		for(int i = 0; i < total_in_buffers; i++)
		{
			delete [] dsp_in[i];
			delete [] ref_channels[i];
			delete [] ref_offsets[i];
			delete [] ref_levels[i];
			delete [] ref_lowpass[i];
			delete [] lowpass_in1[i];
			delete [] lowpass_in2[i];
		}

		delete [] dsp_in;
		delete [] ref_channels;
		delete [] ref_offsets;
		delete [] ref_levels;
		delete [] ref_lowpass;
		delete [] lowpass_in1;
		delete [] lowpass_in2;

		for(int i = 0; i < (smp + 1); i++)
		{
			delete engine[i];
		}
		delete [] engine;
		initialized = 0;
	}
}

PLUGIN_CLASS_METHODS

void Reverb::process_realtime(AFrame **input, AFrame **output)
{
	int new_dsp_length, i, j;
	main_in = input;
	main_out = output;
	int size = input[0]->length;
	redo_buffers |= load_configuration();

	if(!config.ref_total) return;

	if(!initialized)
	{
		dsp_in = new double*[total_in_buffers];
		ref_channels = new int*[total_in_buffers];
		ref_offsets = new int*[total_in_buffers];
		ref_levels = new double*[total_in_buffers];
		ref_lowpass = new int*[total_in_buffers];
		lowpass_in1 = new double*[total_in_buffers];
		lowpass_in2 = new double*[total_in_buffers];

		for(i = 0; i < total_in_buffers; i++)
		{
			dsp_in[i] = new double[1];
			ref_channels[i] = new int[1];
			ref_offsets[i] = new int[1];
			ref_levels[i] = new double[1];
			ref_lowpass[i] = new int[1];
			lowpass_in1[i] = new double[1];
			lowpass_in2[i] = new double[1];
		}

		engine = new ReverbEngine*[(smp + 1)];
		for(i = 0; i < (smp + 1); i++)
		{
			engine[i] = new ReverbEngine(this);
			engine[i]->start();
		}
		initialized = 1;
		redo_buffers = 1;
	}

	new_dsp_length = size + 
		(config.delay_init + config.ref_length) * 
		project_sample_rate / 1000 + 1;

	if(redo_buffers || new_dsp_length != dsp_in_length)
	{
		for(i = 0; i < total_in_buffers; i++)
		{
			double *old_dsp = dsp_in[i];
			double *new_dsp = new double[new_dsp_length];
			for(j = 0; j < dsp_in_length && j < new_dsp_length; j++) 
				new_dsp[j] = old_dsp[j];
			for( ; j < new_dsp_length; j++) new_dsp[j] = 0;
			delete [] old_dsp;
			dsp_in[i] = new_dsp;
		}

		dsp_in_length = new_dsp_length;
		redo_buffers = 1;
	}

	if(redo_buffers)
	{
		for(i = 0; i < total_in_buffers; i++)
		{
			delete [] ref_channels[i];
			delete [] ref_offsets[i];
			delete [] ref_lowpass[i];
			delete [] ref_levels[i];
			delete [] lowpass_in1[i];
			delete [] lowpass_in2[i];

			ref_channels[i] = new int[config.ref_total + 1];
			ref_offsets[i] = new int[config.ref_total + 1];
			ref_lowpass[i] = new int[config.ref_total + 1];
			ref_levels[i] = new double[config.ref_total + 1];
			lowpass_in1[i] = new double[config.ref_total + 1];
			lowpass_in2[i] = new double[config.ref_total + 1];

// set channels
			ref_channels[i][0] = i;         // primary noise
			ref_channels[i][1] = i;         // first reflection
// set offsets
			ref_offsets[i][0] = 0;
			ref_offsets[i][1] = config.delay_init * project_sample_rate / 1000;
// set levels
			ref_levels[i][0] = db.fromdb(config.level_init);
			ref_levels[i][1] = db.fromdb(config.ref_level1);
// set lowpass
			ref_lowpass[i][0] = -1;     // ignore first noise
			ref_lowpass[i][1] = config.lowpass1;
			lowpass_in1[i][0] = 0;
			lowpass_in2[i][0] = 0;
			lowpass_in1[i][1] = 0;
			lowpass_in2[i][1] = 0;

			int ref_division = config.ref_length *
				project_sample_rate / 1000 / (config.ref_total + 1);

			for(j = 2; j < config.ref_total + 1; j++)
			{
// set random channels for remaining reflections
				ref_channels[i][j] = rand() % total_in_buffers;

// set random offsets after first reflection
				ref_offsets[i][j] = ref_offsets[i][1];
				if(ref_division > 0)
					ref_offsets[i][j] += ref_division * j - (rand() % ref_division) / 2;

// set changing levels
				ref_levels[i][j] = db.fromdb(config.ref_level1 + (config.ref_level2 - config.ref_level1) / (config.ref_total - 1) * (j - 2));

// set changing lowpass as linear
				ref_lowpass[i][j] = config.lowpass1 + (double)(config.lowpass2 - config.lowpass1) / (config.ref_total - 1) * (j - 2);
				lowpass_in1[i][j] = 0;
				lowpass_in2[i][j] = 0;
			}
		}
		redo_buffers = 0;
	}

	for(i = 0; i < total_in_buffers; )
	{
		for(j = 0; j < (smp + 1) && (i + j) < total_in_buffers; j++)
		{
			engine[j]->process_overlays(i + j, size);
		}

		for(j = 0; j < (smp + 1) && i < total_in_buffers; j++, i++)
		{
			engine[j]->wait_process_overlays();
		}
	}

	for(i = 0; i < total_in_buffers; i++)
	{
		if(main_out[i] != main_in[i])
			main_out[i]->copy_of(main_in[i]);

		double *current_out = main_out[i]->buffer;
		double *current_in = dsp_in[i];

		for(j = 0; j < size; j++) current_out[j] = current_in[j];

		int k;
		for(k = 0; j < dsp_in_length; j++, k++) 
			current_in[k] = current_in[j];

		for(; k < dsp_in_length; k++) current_in[k] = 0;
	}
}


void Reverb::load_defaults()
{
	defaults = load_defaults_file("reverb.rc");
	config.level_init = defaults->get("LEVEL_INIT", (double)0);
	config.delay_init = defaults->get("DELAY_INIT", 100);
	config.ref_level1 = defaults->get("REF_LEVEL1", (double)-6);
	config.ref_level2 = defaults->get("REF_LEVEL2", (double)INFINITYGAIN);
	config.ref_total = defaults->get("REF_TOTAL", 12);
	config.ref_length = defaults->get("REF_LENGTH", 1000);
	config.lowpass1 = defaults->get("LOWPASS1", 20000);
	config.lowpass2 = defaults->get("LOWPASS2", 20000);
}

void Reverb::save_defaults()
{
	defaults->update("LEVEL_INIT", config.level_init);
	defaults->update("DELAY_INIT", config.delay_init);
	defaults->update("REF_LEVEL1", config.ref_level1);
	defaults->update("REF_LEVEL2", config.ref_level2);
	defaults->update("REF_TOTAL", config.ref_total);
	defaults->update("REF_LENGTH", config.ref_length);
	defaults->update("LOWPASS1", config.lowpass1);
	defaults->update("LOWPASS2", config.lowpass2);
	defaults->save();
}

void Reverb::save_data(KeyFrame *keyframe)
{
	FileXML output;

	output.tag.set_title("REVERB");
	output.tag.set_property("LEVELINIT", config.level_init);
	output.tag.set_property("DELAY_INIT", config.delay_init);
	output.tag.set_property("REF_LEVEL1", config.ref_level1);
	output.tag.set_property("REF_LEVEL2", config.ref_level2);
	output.tag.set_property("REF_TOTAL", config.ref_total);
	output.tag.set_property("REF_LENGTH", config.ref_length);
	output.tag.set_property("LOWPASS1", config.lowpass1);
	output.tag.set_property("LOWPASS2", config.lowpass2);
	output.append_tag();
	output.tag.set_title("/REVERB");
	output.append_tag();
	keyframe->set_data(output.string);
}

void Reverb::read_data(KeyFrame *keyframe)
{
	FileXML input;
	int result = 0;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());

	result = input.read_tag();

	if(!result)
	{
		if(input.tag.title_is("REVERB"))
		{
			config.level_init = input.tag.get_property("LEVELINIT", config.level_init);
			config.delay_init = input.tag.get_property("DELAY_INIT", config.delay_init);
			config.ref_level1 = input.tag.get_property("REF_LEVEL1", config.ref_level1);
			config.ref_level2 = input.tag.get_property("REF_LEVEL2", config.ref_level2);
			config.ref_total = input.tag.get_property("REF_TOTAL", config.ref_total);
			config.ref_length = input.tag.get_property("REF_LENGTH", config.ref_length);
			config.lowpass1 = input.tag.get_property("LOWPASS1", config.lowpass1);
			config.lowpass2 = input.tag.get_property("LOWPASS2", config.lowpass2);
		}
	}
}

ReverbEngine::ReverbEngine(Reverb *plugin)
 : Thread()
{
	this->plugin = plugin;
	completed = 0;
	input_lock.lock();
	output_lock.lock();
}

ReverbEngine::~ReverbEngine()
{
	completed = 1;
	input_lock.unlock();
	join();
}

int ReverbEngine::process_overlays(int output_buffer, int size)
{
	this->output_buffer = output_buffer;
	this->size = size;
	input_lock.unlock();
}

int ReverbEngine::wait_process_overlays()
{
	output_lock.lock();
}

int ReverbEngine::process_overlay(double *in, double *out, double &out1, 
	double &out2, double level, 
	int lowpass, int samplerate, int size)
{
// Modern niquist frequency is 44khz but pot limit is 20khz so can't use
// niquist
	if(lowpass == -1 || lowpass >= 20000)
	{
// no lowpass filter
		for(int i = 0; i < size; i++) out[i] += in[i] * level;
	}
	else
	{
		double coef = 0.25 * 2.0 * M_PI *
			(double)lowpass / (double)plugin->project_sample_rate;
		double a = coef * 0.25;
		double b = coef * 0.50;

		for(int i = 0; i < size; i++)
		{
			out2 += a * (3 * out1 + in[i] - out2);
			out2 += b * (out1 + in[i] - out2);
			out2 += a * (out1 + 3 * in[i] - out2);
			out2 += coef * (in[i] - out2);
			out1 = in[i];
			out[i] += out2 * level;
		}
	}
}

void ReverbEngine::run()
{
	int j, i;
	while(1)
	{
		input_lock.lock();
		if(completed) return;

// Process reverb
		for(i = 0; i < plugin->total_in_buffers; i++)
		{
			for(j = 0; j < plugin->config.ref_total + 1; j++)
			{
				if(plugin->ref_channels[i][j] == output_buffer)
					process_overlay(plugin->main_in[i]->buffer,
						&(plugin->dsp_in[plugin->ref_channels[i][j]][plugin->ref_offsets[i][j]]),
						plugin->lowpass_in1[i][j],
						plugin->lowpass_in2[i][j],
						plugin->ref_levels[i][j],
						plugin->ref_lowpass[i][j],
						plugin->project_sample_rate,
						size);
			}
		}

		output_lock.unlock();
	}
}


ReverbConfig::ReverbConfig()
{
}

int ReverbConfig::equivalent(ReverbConfig &that)
{
	return (EQUIV(level_init, that.level_init) &&
		delay_init == that.delay_init &&
		EQUIV(ref_level1, that.ref_level1) &&
		EQUIV(ref_level2, that.ref_level2) &&
		ref_total == that.ref_total &&
		ref_length == that.ref_length &&
		lowpass1 == that.lowpass1 &&
		lowpass2 == that.lowpass2);
}

void ReverbConfig::copy_from(ReverbConfig &that)
{
	level_init = that.level_init;
	delay_init = that.delay_init;
	ref_level1 = that.ref_level1;
	ref_level2 = that.ref_level2;
	ref_total = that.ref_total;
	ref_length = that.ref_length;
	lowpass1 = that.lowpass1;
	lowpass2 = that.lowpass2;
}

void ReverbConfig::interpolate(ReverbConfig &prev, 
	ReverbConfig &next, 
	ptstime prev_pts,
	ptstime next_pts,
	ptstime current_pts)
{
	level_init = prev.level_init;
	delay_init = prev.delay_init;
	ref_level1 = prev.ref_level1;
	ref_level2 = prev.ref_level2;
	ref_total = prev.ref_total;
	ref_length = prev.ref_length;
	lowpass1 = prev.lowpass1;
	lowpass2 = prev.lowpass2;
}

void ReverbConfig::dump()
{
	printf("ReverbConfig::dump %f %d %f %f %d %d %d %d\n",
	level_init,
	delay_init, 
	ref_level1, 
	ref_level2, 
	ref_total, 
	ref_length, 
	lowpass1, 
	lowpass2);
}
