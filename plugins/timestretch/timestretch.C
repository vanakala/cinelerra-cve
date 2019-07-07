
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
#include "bctitle.h"
#include "picon_png.h"
#include "resample.h"
#include "timestretch.h"
#include "filexml.h"

#include <string.h>


#define WINDOW_SIZE 4096
#define INPUT_SIZE 65536
#define OVERSAMPLE 8

REGISTER_PLUGIN


PitchEngine::PitchEngine(TimeStretch *plugin, int window_size)
 : CrossfadeFFT(window_size)
{
	this->plugin = plugin;
	last_phase = new double[WINDOW_SIZE];
	new_freq = new double[WINDOW_SIZE];
	new_magn = new double[WINDOW_SIZE];
	sum_phase = new double[WINDOW_SIZE];
	anal_magn = new double[WINDOW_SIZE];
	anal_freq = new double[WINDOW_SIZE];

	input_buffer = 0;
	input_size = 0;
	input_allocated = 0;
	current_output_pts = -1;
	temp = 0;
}

PitchEngine::~PitchEngine()
{
	if(input_buffer) delete [] input_buffer;
	if(temp) delete temp;
	delete [] last_phase;
	delete [] new_freq;
	delete [] new_magn;
	delete [] sum_phase;
	delete [] anal_magn;
	delete [] anal_freq;
}

void PitchEngine::get_frame(AFrame *aframe)
{
// FIXME, make sure this is set at the beginning, always
	int samples = aframe->source_length;

	if (!PTSEQU(current_output_pts,  aframe->pts))
	{
		input_size = 0;
		ptstime input_pts = plugin->source_start_pts + 
			(aframe->pts - plugin->source_start_pts) / plugin->config.scale;
		current_input_sample = aframe->to_samples(input_pts);
		current_output_pts = aframe->pts;
	}

	while(input_size < samples)
	{
		double scale = plugin->config.scale;
		if(!temp)
		{
			temp = new AFrame(INPUT_SIZE);
			temp->samplerate = aframe->samplerate;
		}
		temp->set_fill_request(current_input_sample, INPUT_SIZE);
		plugin->get_frame(temp);
		current_input_sample += aframe->length;

		plugin->resample->resample_chunk(temp->buffer,
			INPUT_SIZE,
			temp->samplerate,
			round(temp->samplerate * scale),
			0);

		int fragment_size = plugin->resample->get_output_size(0);

		if(input_size + fragment_size > input_allocated)
		{
			int new_allocated = input_size + fragment_size;
			double *new_buffer = new double[new_allocated];
			if(input_buffer)
			{
				memcpy(new_buffer, input_buffer, input_size * sizeof(double));
				delete [] input_buffer;
			}
			input_buffer = new_buffer;
			input_allocated = new_allocated;
		}

		plugin->resample->read_output(input_buffer + input_size,
			0,
			fragment_size);
		input_size += fragment_size;
	}
	memcpy(aframe->buffer, input_buffer, aframe->source_length * sizeof(double));
	memcpy(input_buffer, 
		input_buffer + aframe->source_length,
		sizeof(double) * (input_size - samples));
	aframe->set_filled(aframe->source_length);
	input_size -= samples;
	current_output_pts = aframe->pts + aframe->duration;
}

void PitchEngine::signal_process_oversample(int reset)
{
	double scale = plugin->config.scale;

	memset(new_freq, 0, window_size * sizeof(double));
	memset(new_magn, 0, window_size * sizeof(double));

	if (reset)
	{
		memset (last_phase, 0, WINDOW_SIZE * sizeof(double));
		memset (sum_phase, 0, WINDOW_SIZE * sizeof(double));
	}

// expected phase difference between windows
	double expected_phase_diff = 2.0 * M_PI / oversample; 
// frequency per bin
	double freq_per_bin = (double)plugin->PluginAClient::project_sample_rate / window_size;

//scale = 1.0;
	for (int i = 0; i < window_size/2; i++) 
	{
// Convert to magnitude and phase
		double magn = sqrt(fftw_data[i][0] * fftw_data[i][0] + fftw_data[i][1] * fftw_data[i][1]);
		double phase = atan2(fftw_data[i][1], fftw_data[i][0]);

// Remember last phase
		double temp = phase - last_phase[i];
		last_phase[i] = phase;

// Substract the expected advancement of phase
		temp -= (double)i * expected_phase_diff;

// wrap temp into -/+ PI ...  good trick!
		int qpd = (int)(temp/M_PI);
		if (qpd >= 0) 
			qpd += qpd&1;
		else 
			qpd -= qpd&1;
		temp -= M_PI*(double)qpd;

// Deviation from bin frequency
		temp = oversample * temp / (2.0 * M_PI);

		temp = (double)(temp + i) * freq_per_bin;

		anal_magn[i] = magn;
		anal_freq[i] = temp;
	}

	for (int k = 0; k <= window_size/2; k++) {
		int index = int(k/scale);
		if (index <= window_size/2)
		{
			new_magn[k] += anal_magn[index];
			new_freq[k] = anal_freq[index] * scale;
		}
		else
		{
			new_magn[k] = 0;
			new_freq[k] = 0;
		}
	}

	// Synthesize back the fft window 
	for (int i = 0; i < window_size/2; i++) 
	{
		double magn = new_magn[i];
		double temp = new_freq[i];
// substract the bin frequency
		temp -= (double)(i) * freq_per_bin;

// get bin deviation from freq deviation
		temp /= freq_per_bin;

// oversample 
		temp = 2.0 * M_PI *temp / oversample;

// add back the expected phase difference (that we substracted in analysis)
		temp += (double)(i) * expected_phase_diff;

// accumulate delta phase, to get bin phase
		sum_phase[i] += temp;

		double phase = sum_phase[i];

		fftw_data[i][0] = magn * cos(phase);
		fftw_data[i][1] = magn * sin(phase);
	}

	for (int i = window_size/2; i< window_size; i++)
	{
		fftw_data[i][0] = 0;
		fftw_data[i][1] = 0;
	}
}


TimeStretch::TimeStretch(PluginServer *server)
 : PluginAClient(server)
{
	PLUGIN_CONSTRUCTOR_MACRO
	temp = 0;
	pitch = 0;
	resample = 0;
	input = 0;
	input_allocated = 0;
}


TimeStretch::~TimeStretch()
{
	PLUGIN_DESTRUCTOR_MACRO
	if(temp) delete [] temp;
	if(input) delete [] input;
	if(pitch) delete pitch;
	if(resample) delete resample;
}

PLUGIN_CLASS_METHODS

void TimeStretch::read_data(KeyFrame *keyframe)
{
	FileXML input;
	int result = 0;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());

	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("TIMESTRETCH"))
			{
				config.scale = input.tag.get_property("SCALE", config.scale);
			}
		}
	}
}

void TimeStretch::save_data(KeyFrame *keyframe)
{
	FileXML output;

	output.tag.set_title("TIMESTRETCH");
	output.tag.set_property("SCALE", config.scale);
	output.append_tag();
	output.tag.set_title("/TIMESTRETCH");
	output.append_tag();
	output.append_newline();
	keyframe->set_data(output.string);
}

void TimeStretch::load_defaults()
{
// load the defaults
	defaults = load_defaults_file("timestretch.rc");

	config.scale = defaults->get("SCALE", (double)1);
}

void TimeStretch::save_defaults()
{
	defaults->update("SCALE", config.scale);
	defaults->save();
}


TimeStretchConfig::TimeStretchConfig()
{
	scale = 1.0;
}

int TimeStretchConfig::equivalent(TimeStretchConfig &that)
{
	return EQUIV(scale, that.scale);
}

void TimeStretchConfig::copy_from(TimeStretchConfig &that)
{
	scale = that.scale;
}

void TimeStretchConfig::interpolate(TimeStretchConfig &prev, 
	TimeStretchConfig &next, 
	ptstime prev_pts,
	ptstime next_pts,
	ptstime current_pts)
{
	PLUGIN_CONFIG_INTERPOLATE_MACRO
	scale = prev.scale * prev_scale + next.scale * next_scale;
}

void TimeStretch::process_frame(AFrame *aframe)
{
	load_configuration();

	if(!pitch)
	{
		pitch = new PitchEngine(this, WINDOW_SIZE);
		pitch->set_oversample(OVERSAMPLE);
		resample = new Resample(0, 1);
	}

	pitch->process_frame_oversample(aframe);
}

PLUGIN_THREAD_METHODS

TimeStretchWindow::TimeStretchWindow(TimeStretch *plugin, int x, int y)
 : PluginWindow(plugin->gui_string, 
	x,
	y, 
	150, 
	50)
{
	x = y = 10;

	add_subwindow(new BC_Title(x, y, _("Scale:")));
	x += 70;
	add_subwindow(scale = new TimeStretchScale(plugin, x, y));
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

void TimeStretchWindow::update()
{
	scale->update(plugin->config.scale);
}


TimeStretchScale::TimeStretchScale(TimeStretch *plugin, int x, int y)
 : BC_FPot(x, y, (float)plugin->config.scale, .3, 2)
{
	this->plugin = plugin;
	set_precision(0.001);
}

int TimeStretchScale::handle_event()
{
	plugin->config.scale = get_value();
	plugin->send_configure_change();
	return 1;
}
