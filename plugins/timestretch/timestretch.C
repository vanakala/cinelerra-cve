#include "bcdisplayinfo.h"
#include "defaults.h"
#include "mainprogress.h"
#include "picon_png.h"
#include "resample.h"
#include "timestretch.h"
#include "vframe.h"

#define WINDOW_SIZE 8192


PluginClient* new_plugin(PluginServer *server)
{
	return new TimeStretch(server);
}




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

	add_subwindow(new BC_Title(x, y, "Fraction of original length:"));
	y += 20;
	add_subwindow(new TimeStretchFraction(plugin, x, y));
	add_subwindow(new BC_OKButton(this));
	add_subwindow(new BC_CancelButton(this));
	show_window();



	flush();
}










PitchEngine::PitchEngine(TimeStretch *plugin)
 : CrossfadeFFT()
{
	this->plugin = plugin;
}


int PitchEngine::signal_process()
{

	int min_freq = 
		1 + (int)(20.0 / ((double)plugin->PluginAClient::project_sample_rate / window_size * 2) + 0.5);

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
//printf("PitchEngine::signal_process 1\n");
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
//printf("PitchEngine::signal_process 1\n");
	}

	symmetry(window_size, freq_real, freq_imag);
//printf("PitchEngine::signal_process 2\n");
	return 0;
}














TimeStretch::TimeStretch(PluginServer *server)
 : PluginAClient(server)
{
	load_defaults();
	temp = 0;
}


TimeStretch::~TimeStretch()
{
	save_defaults();
	delete defaults;
	if(temp) delete [] temp;
}

	
	
char* TimeStretch::plugin_title()
{
	return "Time stretch";
}

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
	if(PluginClient::interactive)
	{
		char string[BCTEXTLEN];
		sprintf(string, "%s...", plugin_title());
		progress = start_progress(string, 
			(long)((double)(PluginClient::end - PluginClient::start) * scale));
	}

	current_position = PluginClient::start;
	total_written = 0;
	
	pitch = new PitchEngine(this);
	pitch->initialize(WINDOW_SIZE);

//printf("TimeStretch::start_loop %d %d\n", (int)(PluginAClient::in_buffer_size * scale + 0.5), WINDOW_SIZE);

	resample = new Resample(0, 1);
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

int TimeStretch::process_loop(double *buffer, long &write_length)
{
	int result = 0;

// Length to read based on desired output size
	long size = (long)((double)PluginAClient::in_buffer_size / scale);
	long predicted_total = (long)((double)(PluginClient::end - PluginClient::start) * scale + 0.5);

	double *input = new double[size];

	read_samples(input, 0, current_position, size);
	current_position += size;

	resample->resample_chunk(input, 
		size, 
		1000000, 
		(int)(1000000.0 * scale), 
		0);


	if(resample->get_output_size(0))
	{
		long output_size = resample->get_output_size(0);
		if(temp && temp_allocated < output_size)
		{
			delete [] temp;
			temp = 0;
		}

		if(!temp)
		{
			temp = new double[output_size];
			temp_allocated = output_size;
		}
		resample->read_output(temp, 0, output_size);
		int samples_rendered = 0;


//printf("TimeStretch::process_loop 1 %d %d\n", output_size, PluginClient::out_buffer_size);
		samples_rendered = pitch->process_fifo(output_size, 
			temp, 
			buffer);
//printf("TimeStretch::process_loop 2 %d %d\n", output_size, PluginClient::out_buffer_size);


		if(samples_rendered)
		{
			total_written += samples_rendered;
		}

// Trim output to predicted length of stretched selection.
		if(total_written > predicted_total)
		{
			samples_rendered -= total_written - predicted_total;
			result = 1;
		}


		write_length = samples_rendered;
		
	}

	if(PluginClient::interactive) result = progress->update(total_written);
//printf("TimeStretch::process_loop 1\n");

	delete [] input;
//printf("TimeStretch::process_loop 2\n");
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
	return 0;
}

int TimeStretch::save_defaults()
{
	defaults->update("SCALE", scale);
	defaults->save();
	return 0;
}
