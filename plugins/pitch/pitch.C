#include "bcdisplayinfo.h"
#include "clip.h"
#include "defaults.h"
#include "filexml.h"
#include "pitch.h"
#include "picon_png.h"
#include "units.h"
#include "vframe.h"

#include <math.h>
#include <string.h>


#include <libintl.h>
#define _(String) gettext(String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)







PluginClient* new_plugin(PluginServer *server)
{
	return new PitchEffect(server);
}





PitchEffect::PitchEffect(PluginServer *server)
 : PluginAClient(server)
{
	reset();
	load_defaults();
}

PitchEffect::~PitchEffect()
{
	if(thread)
	{
		thread->window->set_done(0);
		thread->completion.lock();
		delete thread;
	}
	
	save_defaults();
	delete defaults;

	if(fft) delete fft;
}

VFrame* PitchEffect::new_picon()
{
	return new VFrame(picon_png);
}

char* PitchEffect::plugin_title()
{
	return _("Pitch shift");
}


int PitchEffect::is_realtime()
{
	return 1;
}



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


void PitchEffect::reset()
{
	thread = 0;
	fft = 0;
}

void PitchEffect::update_gui()
{
	if(thread)
	{
		load_configuration();
		thread->window->lock_window();
		thread->window->update();
		thread->window->unlock_window();
	}
}

int PitchEffect::show_gui()
{
	load_configuration();
	
	thread = new PitchThread(this);
	thread->start();
	return 0;
}

void PitchEffect::raise_window()
{
	if(thread)
	{
		thread->window->lock_window();
		thread->window->raise_window();
		thread->window->flush();
		thread->window->unlock_window();
	}
}

int PitchEffect::set_string()
{
	if(thread) 
	{
		thread->window->lock_window();
		thread->window->set_title(gui_string);
		thread->window->unlock_window();
	}
	return 0;
}

int PitchEffect::process_realtime(int64_t size, double *input_ptr, double *output_ptr)
{
	load_configuration();
	if(!fft) fft = new PitchFFT(this);
	fft->process_fifo(size, input_ptr, output_ptr);
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









PitchThread::PitchThread(PitchEffect *plugin)
 : Thread()
{
	this->plugin = plugin;
	set_synchronous(0);
	completion.lock();
}

PitchThread::~PitchThread()
{
	delete window;
}


void PitchThread::run()
{
	BC_DisplayInfo info;

	window = new PitchWindow(plugin,
		info.get_abs_cursor_x() - 125, 
		info.get_abs_cursor_y() - 115);

	window->create_objects();
	int result = window->run_window();
	completion.unlock();
// Last command in thread
	if(result) plugin->client_side_close();
}








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

int PitchWindow::close_event()
{
// Set result to 1 to indicate a client side close
	set_done(1);
	return 1;
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
















