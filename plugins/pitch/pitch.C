
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
#include "filexml.h"
#include "language.h"
#include "pitch.h"
#include "picon_png.h"
#include "units.h"
#include "vframe.h"

#include <math.h>
#include <string.h>



#define WINDOW_SIZE 8192
#define OVERSAMPLE 8


//#define WINDOW_SIZE 131072

REGISTER_PLUGIN(PitchEffect);





PitchEffect::PitchEffect(PluginServer *server)
 : PluginAClient(server)
{
	PLUGIN_CONSTRUCTOR_MACRO
	reset();
}

PitchEffect::~PitchEffect()
{
	PLUGIN_DESTRUCTOR_MACRO

	if(fft) delete fft;
}

char* PitchEffect::plugin_title() { return N_("Pitch shift"); }
int PitchEffect::is_realtime() { return 1; }



void PitchEffect::read_data(KeyFrame *keyframe)
{
	FileXML input;
	input.set_shared_string(keyframe->data, strlen(keyframe->data));

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
	output.set_shared_string(keyframe->data, MESSAGESIZE);

	output.tag.set_title("PITCH");
	output.tag.set_property("SCALE", config.scale);
	output.append_tag();
	output.tag.set_title("/PITCH");
	output.append_tag();
	output.append_newline();

	output.terminate_string();
}

int PitchEffect::load_defaults()
{
	char directory[BCTEXTLEN], string[BCTEXTLEN];
	sprintf(directory, "%spitch.rc", BCASTDIR);
	defaults = new BC_Hash(directory);
	defaults->load();
	
	config.scale = defaults->get("SCALE", config.scale);
	return 0;
}

int PitchEffect::save_defaults()
{
	char string[BCTEXTLEN];

	defaults->update("SCALE", config.scale);
	defaults->save();

	return 0;
}


LOAD_CONFIGURATION_MACRO(PitchEffect, PitchConfig)

SHOW_GUI_MACRO(PitchEffect, PitchThread)

RAISE_WINDOW_MACRO(PitchEffect)

SET_STRING_MACRO(PitchEffect)

NEW_PICON_MACRO(PitchEffect)


void PitchEffect::reset()
{
	fft = 0;
}

void PitchEffect::update_gui()
{
	if(thread)
	{
		load_configuration();
		thread->window->lock_window("PitchEffect::update_gui");
		thread->window->update();
		thread->window->unlock_window();
	}
}



int PitchEffect::process_buffer(int64_t size, 
		double *buffer,
		int64_t start_position,
		int sample_rate)
{
	load_configuration();


	if(!fft)
	{
		fft = new PitchFFT(this);
		fft->initialize(WINDOW_SIZE);
		fft->set_oversample(OVERSAMPLE);
	}

	fft->process_buffer_oversample(start_position,
		size, 
		buffer,
		get_direction());

	return 0;
}









PitchFFT::PitchFFT(PitchEffect *plugin)
 : CrossfadeFFT()
{
	this->plugin = plugin;
	last_phase = new double[WINDOW_SIZE];
	new_freq = new double[WINDOW_SIZE];
	new_magn = new double[WINDOW_SIZE];
	sum_phase = new double[WINDOW_SIZE];
	anal_magn = new double[WINDOW_SIZE];
	anal_freq = new double[WINDOW_SIZE];

}

PitchFFT::~PitchFFT()
{
	delete [] last_phase;
	delete [] new_freq;
	delete [] new_magn;
	delete [] sum_phase;
	delete [] anal_magn;
	delete [] anal_freq;
}


int PitchFFT::signal_process_oversample(int reset)
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

//		anal_magn[i] = magn;
//		anal_freq[i] = temp;

// Now temp is the real freq... move it!
//		int new_bin = (int)(temp * scale / freq_per_bin + 0.5);
		int new_bin = (int)(i * scale);
		if (new_bin >= 0 && new_bin < window_size/2)
		{
//			double tot_magn = new_magn[new_bin] + magn;

//			new_freq[new_bin] = (new_freq[new_bin] * new_magn[new_bin] + temp *scale* magn) / tot_magn;
			new_freq[new_bin] = temp*scale;
			new_magn[new_bin] += magn;
		}

	}

/*	for (int k = 0; k <= window_size/2; k++) {
		int index = k/scale;
		if (index <= window_size/2) {
			new_magn[k] += anal_magn[index];
			new_freq[k] = anal_freq[index] * scale;
		} else{

		new_magn[k] = 0;
		new_freq[k] = 0;
		}
	}

*/
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

//symmetry(window_size, freq_real, freq_imag);
	for (int i = window_size/2; i< window_size; i++)
	{
		fftw_data[i][0] = 0;
		fftw_data[i][1] = 0;
	}
	

	return 0;
}

int PitchFFT::read_samples(int64_t output_sample, 
	int samples, 
	double *buffer)
{
	return plugin->read_samples(buffer,
		0,
		plugin->get_samplerate(),
		output_sample,
		samples);
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
	int64_t prev_frame, 
	int64_t next_frame, 
	int64_t current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);
	scale = prev.scale * prev_scale + next.scale * next_scale;
}








PLUGIN_THREAD_OBJECT(PitchEffect, PitchThread, PitchWindow) 









PitchWindow::PitchWindow(PitchEffect *plugin, int x, int y)
 : BC_Window(plugin->gui_string, 
 	x, 
	y, 
	150, 
	50, 
	150, 
	50,
	0, 
	0,
	1)
{
	this->plugin = plugin;
}

void PitchWindow::create_objects()
{
	int x = 10, y = 10;
	
	add_subwindow(new BC_Title(x, y, _("Scale:")));
	x += 70;
	add_subwindow(scale = new PitchScale(plugin, x, y));
	show_window();
	flush();
}

WINDOW_CLOSE_EVENT(PitchWindow)

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
















