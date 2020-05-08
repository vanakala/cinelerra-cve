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
#include "vframe.h"

#include <math.h>
#include <string.h>

#define OVERSAMPLE 0

REGISTER_PLUGIN


PitchEffect::PitchEffect(PluginServer *server)
 : PluginAClient(server)
{
	PLUGIN_CONSTRUCTOR_MACRO
	fft = 0;
	input_frame = 0;
}

PitchEffect::~PitchEffect()
{
	PLUGIN_DESTRUCTOR_MACRO
	delete fft;
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
	input_frame = aframe;

	if(!fft)
		fft = new PitchFFT(this, aframe->get_buffer_length() / 2);
	else if(need_reconfigure)
		fft->reset_phase = 1;

	aframe = fft->process_frame(aframe);
	return aframe;
}


PitchFFT::PitchFFT(PitchEffect *plugin, int window_size)
 : Fourier(window_size)
{
	this->plugin = plugin;
	oversample = OVERSAMPLE;
	reset_phase = 1;
	last_phase = new double[window_size];
	new_freq = new double[window_size];
	new_magn = new double[window_size];
	sum_phase = new double[window_size];
}

PitchFFT::~PitchFFT()
{
	delete [] last_phase;
	delete [] new_freq;
	delete [] new_magn;
	delete [] sum_phase;
}

int PitchFFT::signal_process()
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
		reset_phase = 0;
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
		int qpd = (int)(temp / M_PI);
		if (qpd >= 0) 
			qpd += qpd&1;
		else 
			qpd -= qpd&1;
		temp -= M_PI*(double)qpd;

// Deviation from bin frequency
		if(oversample)
			temp = oversample * temp / (2.0 * M_PI);

		temp = (double)(temp + i) * freq_per_bin;

// Now temp is the real freq... move it!
		int new_bin = (int)(i * scale);
		if(new_bin >= 0 && new_bin < half_size)
		{
			new_freq[new_bin] = temp * scale;
			new_magn[new_bin] += magn;
		}
	}
// Synthesize back the fft window
	for (int i = 0; i < half_size; i++)
	{
		double magn = new_magn[i];
		double temp = new_freq[i];
// substract the bin frequency
		temp -= (double)(i) * freq_per_bin;

// get bin deviation from freq deviation
		temp /= freq_per_bin;

		if(oversample)
		{
			temp = 2.0 * M_PI * temp / oversample;
// add back the expected phase difference (that we substracted in analysis)
			temp += (double)(i) * expected_phase_diff;
		}

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
	return 1;
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
 : BC_FPot(x, y, (float)plugin->config.scale, .5, 1.5)
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
