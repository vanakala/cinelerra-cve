#include "bcdisplayinfo.h"
#include "clip.h"
#include "defaults.h"
#include "language.h"
#include "mainprogress.h"
#include "picon_png.h"
#include "resample.h"
#include "timestretch.h"
#include "timestretchengine.h"
#include "transportque.inc"
#include "vframe.h"

#include <string.h>


#define WINDOW_SIZE 4096
#define INPUT_SIZE 65536


REGISTER_PLUGIN(TimeStretch)





TimeStretchFraction::TimeStretchFraction(TimeStretch *plugin, int x, int y)
 : BC_TextBox(x, y, 100, 1, (float)plugin->scale)
{
	this->plugin = plugin;
}

int TimeStretchFraction::handle_event()
{
	plugin->scale = atof(get_text());
	return 1;
}





TimeStretchFreq::TimeStretchFreq(TimeStretch *plugin, 
	TimeStretchWindow *gui, 
	int x, 
	int y)
 : BC_Radial(x, y, plugin->use_fft, _("Use fast fourier transform"))
{
	this->plugin = plugin;
	this->gui = gui;
}

int TimeStretchFreq::handle_event()
{
	plugin->use_fft = 1;
	update(1);
	gui->time->update(0);
}






TimeStretchTime::TimeStretchTime(TimeStretch *plugin, 
	TimeStretchWindow *gui, 
	int x, 
	int y)
 : BC_Radial(x, y, !plugin->use_fft, _("Use overlapping windows"))
{
	this->plugin = plugin;
	this->gui = gui;
}

int TimeStretchTime::handle_event()
{
	plugin->use_fft = 0;
	update(1);
	gui->freq->update(0);
}












TimeStretchWindow::TimeStretchWindow(TimeStretch *plugin, int x, int y)
 : BC_Window(PROGRAM_NAME ": Time stretch", 
 				x - 160,
				y - 75,
 				320, 
				150, 
				320, 
				150,
				0,
				0,
				1)
{
	this->plugin = plugin;
}


TimeStretchWindow::~TimeStretchWindow()
{
}

void TimeStretchWindow::create_objects()
{
	int x = 10, y = 10;

	add_subwindow(new BC_Title(x, y, _("Fraction of original length:")));
	y += 20;
	add_subwindow(new TimeStretchFraction(plugin, x, y));

	y += 30;
	add_subwindow(freq = new TimeStretchFreq(plugin, this, x, y));
	y += 20;
	add_subwindow(time = new TimeStretchTime(plugin, this, x, y));

	add_subwindow(new BC_OKButton(this));
	add_subwindow(new BC_CancelButton(this));
	show_window();



	flush();
}










PitchEngine::PitchEngine(TimeStretch *plugin)
 : CrossfadeFFT()
{
	this->plugin = plugin;
	input_buffer = 0;
	input_size = 0;
	input_allocated = 0;
	current_position = 0;
	temp = 0;
}

PitchEngine::~PitchEngine()
{
	if(input_buffer) delete [] input_buffer;
	if(temp) delete [] temp;
}

int PitchEngine::read_samples(int64_t output_sample, 
	int samples, 
	double *buffer)
{
	while(input_size < samples)
	{
		if(!temp) temp = new double[INPUT_SIZE];

		plugin->read_samples(temp, 
			0, 
			plugin->get_source_start() + current_position, 
			INPUT_SIZE);
		current_position +=INPUT_SIZE;

		plugin->resample->resample_chunk(temp,
			INPUT_SIZE,
			1000000,
			(int)(1000000 * plugin->scale),
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
	return 0;
}

int PitchEngine::signal_process()
{

	int min_freq = 
		1 + (int)(20.0 / 
				((double)plugin->PluginAClient::project_sample_rate / 
					window_size * 
					2) + 
				0.5);

	if(plugin->scale < 1)
	{
		for(int i = min_freq; i < window_size / 2; i++)
		{
			double destination = i * plugin->scale;
			int dest_i = (int)(destination + 0.5);
			if(dest_i != i)
			{
				if(dest_i <= window_size / 2)
				{
					freq_real[dest_i] = freq_real[i];
					freq_imag[dest_i] = freq_imag[i];
				}
				freq_real[i] = 0;
				freq_imag[i] = 0;
			}
		}
	}
	else
	if(plugin->scale > 1)
	{
		for(int i = window_size / 2 - 1; i >= min_freq; i--)
		{
			double destination = i * plugin->scale;
			int dest_i = (int)(destination + 0.5);
			if(dest_i != i)
			{
				if(dest_i <= window_size / 2)
				{
					freq_real[dest_i] = freq_real[i];
					freq_imag[dest_i] = freq_imag[i];
				}
				freq_real[i] = 0;
				freq_imag[i] = 0;
			}
		}
	}

	symmetry(window_size, freq_real, freq_imag);
	return 0;
}














TimeStretch::TimeStretch(PluginServer *server)
 : PluginAClient(server)
{
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
	save_defaults();
	delete defaults;
	if(temp) delete [] temp;
	if(input) delete [] input;
	if(pitch) delete pitch;
	if(resample) delete resample;
	if(stretch) delete stretch;
}

	
	
char* TimeStretch::plugin_title() { return N_("Time stretch"); }

int TimeStretch::get_parameters()
{
	BC_DisplayInfo info;
	TimeStretchWindow window(this, info.get_abs_cursor_x(), info.get_abs_cursor_y());
	window.create_objects();
	int result = window.run_window();
	
	return result;
}

VFrame* TimeStretch::new_picon()
{
	return new VFrame(picon_png);
}


int TimeStretch::start_loop()
{
	scaled_size = (int64_t)(get_total_len() * scale);
	if(PluginClient::interactive)
	{
		char string[BCTEXTLEN];
		sprintf(string, "%s...", plugin_title());
		progress = start_progress(string, scaled_size);
	}

	current_position = get_source_start();
	total_written = 0;
	total_read = 0;



// The FFT case
	if(use_fft)
	{
		pitch = new PitchEngine(this);
		pitch->initialize(WINDOW_SIZE);
		resample = new Resample(0, 1);
	}
	else
// The windowing case
	{
// Must be short enough to mask beating but long enough to mask humming.
		stretch = new TimeStretchEngine(scale, PluginAClient::project_sample_rate);
	}



	
	return 0;
}

int TimeStretch::stop_loop()
{
	if(PluginClient::interactive)
	{
		progress->stop_progress();
		delete progress;
	}
	return 0;
}

int TimeStretch::process_loop(double *buffer, int64_t &write_length)
{
	int result = 0;
	int64_t predicted_total = (int64_t)((double)get_total_len() * scale + 0.5);






	int samples_rendered = 0;







// The FFT case
	if(use_fft)
	{
		samples_rendered = get_buffer_size();
		pitch->process_buffer(total_written,
					samples_rendered, 
					buffer, 
					PLAY_FORWARD);
	}
	else
// The windowing case
	{
// Length to read based on desired output size
		int64_t size = (int64_t)((double)get_buffer_size() / scale);

		if(input_allocated < size)
		{
			if(input) delete [] input;
			input = new double[size];
			input_allocated = size;
		}

		read_samples(input, 0, current_position, size);
		current_position += size;

		samples_rendered = stretch->process(input, size);
		if(samples_rendered)
		{
			samples_rendered = MIN(samples_rendered, get_buffer_size());
			stretch->read_output(buffer, samples_rendered);
		}
	}


	total_written += samples_rendered;

// Trim output to predicted length of stretched selection.
	if(total_written > predicted_total)
	{
		samples_rendered -= total_written - predicted_total;
		result = 1;
	}


	write_length = samples_rendered;
	if(PluginClient::interactive) result = progress->update(total_written);

	return result;
}



int TimeStretch::load_defaults()
{
	char directory[BCTEXTLEN];

// set the default directory
	sprintf(directory, "%stimestretch.rc", BCASTDIR);
// load the defaults
	defaults = new Defaults(directory);
	defaults->load();

	scale = defaults->get("SCALE", (double)1);
	use_fft = defaults->get("USE_FFT", 0);
	return 0;
}

int TimeStretch::save_defaults()
{
	defaults->update("SCALE", scale);
	defaults->update("USE_FFT", use_fft);
	defaults->save();
	return 0;
}
