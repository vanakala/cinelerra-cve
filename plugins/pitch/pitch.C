#include "bcdisplayinfo.h"
#include "clip.h"
#include "defaults.h"
#include "filexml.h"
#include "language.h"
#include "pitch.h"
#include "picon_png.h"
#include "units.h"
#include "vframe.h"

#include <math.h>
#include <string.h>



#define WINDOW_SIZE 4096



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
	output.append_newline();

	output.terminate_string();
}

int PitchEffect::load_defaults()
{
	char directory[BCTEXTLEN], string[BCTEXTLEN];
	sprintf(directory, "%spitch.rc", BCASTDIR);
	defaults = new Defaults(directory);
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
	}
	fft->process_buffer(start_position,
		size, 
		buffer,
		get_direction());
	return 0;
}









PitchFFT::PitchFFT(PitchEffect *plugin)
 : CrossfadeFFT()
{
	this->plugin = plugin;
}


int PitchFFT::signal_process()
{
	int min_freq = 
		1 + (int)(20.0 / ((double)plugin->PluginAClient::project_sample_rate / window_size * 2) + 0.5);
//printf("PitchFFT::signal_process %d\n", min_freq);
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
					freq_real[dest_i] = freq_real[i];
					freq_imag[dest_i] = freq_imag[i];
				}
				freq_real[i] = 0;
				freq_imag[i] = 0;
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
















