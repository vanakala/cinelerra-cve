
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
#include "language.h"
#include "mainprogress.h"
#include "picon_png.h"
#include "resample.h"
#include "timestretch.h"
#include "timestretchengine.h"
#include "transportque.inc"
#include "vframe.h"
#include "filexml.h"

#include <string.h>


#define WINDOW_SIZE 4096
#define INPUT_SIZE 65536
#define OVERSAMPLE 8


REGISTER_PLUGIN(TimeStretch)



PitchEngine::PitchEngine(TimeStretch *plugin)
 : CrossfadeFFT()
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
	current_output_sample = -100000000000LL;
	temp = 0;
}

PitchEngine::~PitchEngine()
{
	if(input_buffer) delete [] input_buffer;
	if(temp) delete [] temp;
	delete [] last_phase;
	delete [] new_freq;
	delete [] new_magn;
	delete [] sum_phase;
	delete [] anal_magn;
	delete [] anal_freq;
}

int PitchEngine::read_samples(int64_t output_sample, 
	int samples, 
	double *buffer)
{

// FIXME, make sure this is set at the beginning, always
// FIXME: we need to do backward play also
	if (current_output_sample != output_sample)
	{
		input_size = 0;
		double input_point = plugin->get_source_start() + (output_sample - plugin->get_source_start()) / plugin->config.scale;
		current_input_sample = plugin->local_to_edl((int64_t)input_point);
		current_output_sample = output_sample;

	}

	while(input_size < samples)
	{
		double scale = plugin->config.scale;
		if(!temp) temp = new double[INPUT_SIZE];

		plugin->read_samples(temp, 
			0, 
			plugin->get_samplerate(),
			current_input_sample, 
			INPUT_SIZE);
		current_input_sample +=INPUT_SIZE;

		plugin->resample->resample_chunk(temp,
			INPUT_SIZE,
			1000000,
			(int)(1000000 * scale),
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
	memcpy(buffer, input_buffer, samples * sizeof(int64_t));
	memcpy(input_buffer, 
		input_buffer + samples, 
		sizeof(int64_t) * (input_size - samples));
	input_size -= samples;
	current_output_sample += samples;
	return 0;
}

int PitchEngine::signal_process_oversample(int reset)
{
	double scale = plugin->config.scale;
	
	memset(new_freq, 0, window_size * sizeof(double));
	memset(new_magn, 0, window_size * sizeof(double));
	
	if (reset)
	{
		memset (last_phase, 0, WINDOW_SIZE * sizeof(double));
		memset (sum_phase, 0, WINDOW_SIZE * sizeof(double));
	}
	
	// new or old behaviour
	if (1)
	{	
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

	// Now temp is the real freq... move it!
	//		int new_bin = (int)(temp * scale / freq_per_bin + 0.5);
	/*		int new_bin = (int)(i *scale);
			if (new_bin >= 0 && new_bin < window_size/2)
			{
	//			double tot_magn = new_magn[new_bin] + magn;
				
	//			new_freq[new_bin] = (new_freq[new_bin] * new_magn[new_bin] + temp *scale* magn) / tot_magn;
				new_freq[new_bin] = temp*scale;
				new_magn[new_bin] += magn;
			}
*/
		}
		
		for (int k = 0; k <= window_size/2; k++) {
			int index = int(k/scale);
			if (index <= window_size/2) {
				new_magn[k] += anal_magn[index];
				new_freq[k] = anal_freq[index] * scale;
			} else{
			
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
	} else
	{
		int min_freq = 
			1 + (int)(20.0 / ((double)plugin->PluginAClient::project_sample_rate / 
				window_size * 2) + 0.5);
		if(plugin->config.scale < 1)
		{
			for(int i = min_freq; i < window_size / 2; i++)
			{
				double destination = i * plugin->config.scale;
				int dest_i = (int)(destination + 0.5);
				if(dest_i != i)
				{
					if(dest_i <= window_size / 2)
					{
						fftw_data[dest_i][0] = fftw_data[i][0];
						fftw_data[dest_i][1] = fftw_data[i][1];
					}
					fftw_data[i][0] = 0;
					fftw_data[i][1] = 0;
				}
			}
		}
		else
		if(plugin->config.scale > 1)
		{
			for(int i = window_size / 2 - 1; i >= min_freq; i--)
			{
				double destination = i * plugin->config.scale;
				int dest_i = (int)(destination + 0.5);
				if(dest_i != i)
				{
					if(dest_i <= window_size / 2)
					{
						fftw_data[dest_i][0] = fftw_data[i][0];
						fftw_data[dest_i][1] = fftw_data[i][1];
					}
					fftw_data[i][0] = 0;
					fftw_data[i][1] = 0;
				}
			}
		}
	}

//symmetry(window_size, freq_real, freq_imag);
	for (int i = window_size/2; i< window_size; i++)
	{
		fftw_data[i][0] = 0;
		fftw_data[i][1] = 0;
	}
	

	return 0;
}














TimeStretch::TimeStretch(PluginServer *server)
 : PluginAClient(server)
{
	PLUGIN_CONSTRUCTOR_MACRO
	load_defaults();
	temp = 0;
	pitch = 0;
	resample = 0;
	stretch = 0;
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
	if(stretch) delete stretch;
}

	
	
char* TimeStretch::plugin_title() { return N_("Time stretch"); }
int TimeStretch::is_realtime() { return 1; }

void TimeStretch::read_data(KeyFrame *keyframe)
{
	FileXML input;
	input.set_shared_string(keyframe->data, strlen(keyframe->data));

	int result = 0;
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
	output.set_shared_string(keyframe->data, MESSAGESIZE);

	output.tag.set_title("TIMESTRETCH");
	output.tag.set_property("SCALE", config.scale);
	output.append_tag();
	output.tag.set_title("/TIMESTRETCH");
	output.append_tag();
	output.append_newline();

	output.terminate_string();
}



int TimeStretch::load_defaults()
{
	char directory[BCTEXTLEN];

// set the default directory
	sprintf(directory, "%stimestretch.rc", BCASTDIR);
// load the defaults
	defaults = new BC_Hash(directory);
	defaults->load();

	config.scale = defaults->get("SCALE", (double)1);
	return 0;
}

int TimeStretch::save_defaults()
{
	defaults->update("SCALE", config.scale);
	defaults->save();
	return 0;
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
	int64_t prev_frame, 
	int64_t next_frame, 
	int64_t current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);
	scale = prev.scale * prev_scale + next.scale * next_scale;
}




LOAD_CONFIGURATION_MACRO(TimeStretch, TimeStretchConfig)

SHOW_GUI_MACRO(TimeStretch, TimeStretchThread)

RAISE_WINDOW_MACRO(TimeStretch)

SET_STRING_MACRO(TimeStretch)

NEW_PICON_MACRO(TimeStretch)


void TimeStretch::update_gui()
{
	if(thread)
	{
		load_configuration();
		thread->window->lock_window("TimeStretch::update_gui");
		thread->window->update();
		thread->window->unlock_window();
	}
}



int TimeStretch::get_parameters()
{
	BC_DisplayInfo info;
	TimeStretchWindow window(this, info.get_abs_cursor_x(), info.get_abs_cursor_y());
	window.create_objects();
	int result = window.run_window();
	
	return result;
}


//int TimeStretch::process_loop(double *buffer, int64_t &write_length)
int TimeStretch::process_buffer(int64_t size, 
		double *buffer,
		int64_t start_position,
		int sample_rate)
{
	load_configuration();

	int result = 0;

	if(!pitch)
	{
		pitch = new PitchEngine(this);
		pitch->initialize(WINDOW_SIZE);
		pitch->set_oversample(OVERSAMPLE);
		resample = new Resample(0, 1);

	}

	pitch->process_buffer_oversample(start_position,
		size, 
		buffer,
		get_direction());


	return result;
}



PLUGIN_THREAD_OBJECT(TimeStretch, TimeStretchThread, TimeStretchWindow) 


TimeStretchWindow::TimeStretchWindow(TimeStretch *plugin, int x, int y)
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

void TimeStretchWindow::create_objects()
{
	int x = 10, y = 10;
	
	add_subwindow(new BC_Title(x, y, _("Scale:")));
	x += 70;
	add_subwindow(scale = new TimeStretchScale(plugin, x, y));
	show_window();
	flush();
}

WINDOW_CLOSE_EVENT(TimeStretchWindow)

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









