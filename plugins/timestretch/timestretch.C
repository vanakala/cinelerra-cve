// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "aframe.h"
#include "clip.h"
#include "bchash.h"
#include "bctitle.h"
#include "picon_png.h"
#include "resample.h"
#include "timestretch.h"
#include "filexml.h"

#include <string.h>

#define OVERSAMPLE 0

REGISTER_PLUGIN


PitchEngine::PitchEngine(TimeStretch *plugin, int window_size)
 : Fourier(window_size)
{
	this->plugin = plugin;
	oversample = OVERSAMPLE;
	reset_phase = 1;
	last_phase = new double[window_size];
	new_freq = new double[window_size];
	new_magn = new double[window_size];
	sum_phase = new double[window_size];
	anal_magn = new double[window_size];
	anal_freq = new double[window_size];
}

PitchEngine::~PitchEngine()
{
	delete [] last_phase;
	delete [] new_freq;
	delete [] new_magn;
	delete [] sum_phase;
	delete [] anal_magn;
	delete [] anal_freq;
}


int PitchEngine::signal_process()
{
	int window_size = get_window_size();
	int half_size = window_size / 2;
	double scale = plugin->config.scale;
	double expected_phase_diff;

	memset(new_freq, 0, window_size * sizeof(double));
	memset(new_magn, 0, window_size * sizeof(double));

	if(reset_phase)
	{
		memset(last_phase, 0, window_size * sizeof(double));
		memset(sum_phase, 0, window_size * sizeof(double));
	}

// expected phase difference between windows
	if(oversample)
		expected_phase_diff = 2.0 * M_PI / oversample;
	else
		expected_phase_diff = 0;
// frequency per bin
	double freq_per_bin = (double)plugin->input_frame->get_samplerate() / window_size;

	for(int i = 0; i < half_size; i++)
	{
// Convert to magnitude and phase
		double re = fftw_window[i][0];
		double im = fftw_window[i][1];

		double magn = sqrt(re * re + im * im);
		double phase = atan2(im, re);

// Remember last phase
		double temp = phase - last_phase[i];
		last_phase[i] = phase;

// Substract the expected advancement of phase
		if(oversample)
			temp -= (double)i * expected_phase_diff;

// wrap temp into -/+ PI ...  good trick!
		int qpd = (int)(temp/M_PI);
		if (qpd >= 0) 
			qpd += qpd&1;
		else 
			qpd -= qpd&1;
		temp -= M_PI*(double)qpd;

// Deviation from bin frequency
		if(oversample)
			temp = oversample * temp / (2.0 * M_PI);

		temp = (double)(temp + i) * freq_per_bin;

		anal_magn[i] = magn;
		anal_freq[i] = temp;
	}

	for(int k = 0; k <= half_size; k++)
	{
		int index = int(k / scale);

		if(index <= half_size)
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
	for(int i = 0; i < half_size; i++)
	{
		double magn = new_magn[i];
		double temp = new_freq[i];
// substract the bin frequency
		temp -= (double)(i) * freq_per_bin;

// get bin deviation from freq deviation
		temp /= freq_per_bin;

		if(oversample)
			temp = 2.0 * M_PI * temp / oversample;

// add back the expected phase difference (that we substracted in analysis)
		if(oversample)
			temp += i * expected_phase_diff;

// accumulate delta phase, to get bin phase
		sum_phase[i] += temp;

		double phase = sum_phase[i];

		fftw_window[i][0] = magn * cos(phase);
		fftw_window[i][1] = magn * sin(phase);
	}

	for(int i = half_size; i < window_size; i++)
	{
		fftw_window[i][0] = 0;
		fftw_window[i][1] = 0;
	}
}


TimeStretch::TimeStretch(PluginServer *server)
 : PluginAClient(server)
{
	PLUGIN_CONSTRUCTOR_MACRO
	pitch = 0;
	resample = 0;
	input_pts = -1;
	prev_input = -1;
	prev_frame = -1;
}


TimeStretch::~TimeStretch()
{
	PLUGIN_DESTRUCTOR_MACRO
	delete pitch;
	delete resample;
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

int TimeStretch::load_configuration()
{
	return 0;
}

AFrame *TimeStretch::process_tmpframe(AFrame *aframe)
{
	int output_pos = 0;
	int fragment_size;
	AFrame *src_frame, *tmp_frame;

	input_frame = aframe;
	src_frame = aframe;

	if(!pitch)
	{
		pitch = new PitchEngine(this, aframe->get_buffer_length() / 4);
		resample = new Resample(0, 1);
		need_reconfigure = 1;
	}
	calculate_pts();
	input_pts = aframe->round_to_sample(input_pts);

	if(need_reconfigure || input_pts < prev_frame || input_pts > prev_input + EPSILON)
	{
		pitch->reset_phase = 1;
		resample->reset(0);
		need_reconfigure = 0;
		prev_input = -1;
	}

	if(!PTSEQU(aframe->get_pts(), input_pts))
		src_frame = 0;
	tmp_frame = 0;

	while(output_pos < aframe->get_length())
	{
		if(src_frame)
		{
			resample->resample_chunk(src_frame->buffer,
				src_frame->get_length(),
				src_frame->get_samplerate(),
				round(src_frame->get_samplerate() * config.scale), 0);
			prev_frame = src_frame->get_pts();
			prev_input = src_frame->get_end_pts();
		}
		fragment_size = resample->get_output_size(0);

		if(fragment_size > 0)
		{
			if(fragment_size > aframe->get_length() - output_pos)
				fragment_size = aframe->get_length() - output_pos;
			resample->read_output(aframe->buffer + output_pos, 0,
				fragment_size);
			output_pos += fragment_size;
		}

		if(prev_input < 0)
			prev_input = input_pts;

		if(output_pos < aframe->get_length())
		{
			if(!tmp_frame)
				tmp_frame = clone_aframe(aframe);
			tmp_frame->set_fill_request(prev_input,
				tmp_frame->get_buffer_length());
			tmp_frame = get_frame(tmp_frame);
			src_frame = tmp_frame;
		}
	}
	release_aframe(tmp_frame);
	aframe = pitch->process_frame(aframe);
	return aframe;
}

void TimeStretch::calculate_pts()
{
	KeyFrame *keyframe;
	ptstime pts;

	keyframe = get_first_keyframe();
	input_pts = pts = get_start();

	if(keyframe)
	{
		read_data(keyframe);
		keyframe = (KeyFrame*)keyframe->next;

		while(keyframe && keyframe->pos_time < source_pts)
		{
			input_pts += (keyframe->pos_time - pts) / config.scale;
			pts = keyframe->pos_time;
			read_data(keyframe);
			keyframe = (KeyFrame*)keyframe->next;
		}
	}
	input_pts += (source_pts - pts) / config.scale;
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
