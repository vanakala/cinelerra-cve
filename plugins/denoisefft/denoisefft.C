#include "bcdisplayinfo.h"
#include "clip.h"
#include "defaults.h"
#include "filesystem.h"
#include "filexml.h"
#include "guicast.h"
#include "mutex.h"
#include "fourier.h"
#include "picon_png.h"
#include "pluginaclient.h"
#include "units.h"
#include "vframe.h"

#include <errno.h>
#include <math.h>
#include <string.h>







class DenoiseFFTEffect;
class DenoiseFFTWindow;
#define REFERENCE_FILE BCASTDIR "denoise.ref"




class DenoiseFFTConfig
{
public:
	DenoiseFFTConfig();
	void copy_from(DenoiseFFTConfig &that);
	int equivalent(DenoiseFFTConfig &that);
	void interpolate(DenoiseFFTConfig &prev, 
		DenoiseFFTConfig &next, 
		int64_t prev_frame, 
		int64_t next_frame, 
		int64_t current_frame);

	int window_size;
	int collecting;
	double level;
};

class DenoiseFFTCollecting : public BC_CheckBox
{
public:
	DenoiseFFTCollecting(DenoiseFFTEffect *plugin, int x, int y);
	int handle_event();
	DenoiseFFTEffect *plugin;
};

class DenoiseFFTLevel : public BC_FPot
{
public:
	DenoiseFFTLevel(DenoiseFFTEffect *plugin, int x, int y);
	int handle_event();
	DenoiseFFTEffect *plugin;
};

// class DenoiseFFTSize : public BC_TextBox
// {
// public:
// 	DenoiseFFTSize(DenoiseFFTEffect *plugin, int x, int y);
// 	int handle_event();
// 	DenoiseFFTEffect *plugin;
// };

class DenoiseFFTWindow : public BC_Window
{
public:
	DenoiseFFTWindow(DenoiseFFTEffect *plugin, int x, int y);
	void create_objects();
	int close_event();
	DenoiseFFTLevel *level;
	DenoiseFFTCollecting *collect;
//	DenoiseFFTSize *size;
	DenoiseFFTEffect *plugin;
};













PLUGIN_THREAD_HEADER(DenoiseFFTEffect, DenoiseFFTThread, DenoiseFFTWindow)


class DenoiseFFTRemove : public CrossfadeFFT
{
public:
	DenoiseFFTRemove(DenoiseFFTEffect *plugin);
	int signal_process();
	DenoiseFFTEffect *plugin;
};

class DenoiseFFTCollect : public CrossfadeFFT
{
public:
	DenoiseFFTCollect(DenoiseFFTEffect *plugin);
	int signal_process();
	DenoiseFFTEffect *plugin;
};

class DenoiseFFTEffect : public PluginAClient
{
public:
	DenoiseFFTEffect(PluginServer *server);
	~DenoiseFFTEffect();

	int is_realtime();
	void read_data(KeyFrame *keyframe);
	void save_data(KeyFrame *keyframe);
	int process_realtime(int64_t size, double *input_ptr, double *output_ptr);




	int load_defaults();
	int save_defaults();
	void reset();
	void update_gui();

	void process_window();


	PLUGIN_CLASS_MEMBERS(DenoiseFFTConfig, DenoiseFFTThread)

	int is_collecting;
	int prev_window_size;
	double *reference;
	DenoiseFFTRemove *remove_engine;
	DenoiseFFTCollect *collect_engine;
};










REGISTER_PLUGIN(DenoiseFFTEffect)









DenoiseFFTConfig::DenoiseFFTConfig()
{
	window_size = 16384;
	collecting = 0;
	level = 0.0;
}

void DenoiseFFTConfig::copy_from(DenoiseFFTConfig &that)
{
	window_size = that.window_size;
	collecting = that.collecting;
	level = that.level;
}

int DenoiseFFTConfig::equivalent(DenoiseFFTConfig &that)
{
	return EQUIV(level, that.level) &&
		window_size == that.window_size &&
		collecting == that.collecting;
}

void DenoiseFFTConfig::interpolate(DenoiseFFTConfig &prev, 
	DenoiseFFTConfig &next, 
	int64_t prev_frame, 
	int64_t next_frame, 
	int64_t current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);
	this->level = prev.level * prev_scale + next.level * next_scale;
	this->window_size = prev.window_size;
	this->collecting = prev.collecting;
}















DenoiseFFTLevel::DenoiseFFTLevel(DenoiseFFTEffect *plugin, int x, int y)
 : BC_FPot(x, y, (float)plugin->config.level, INFINITYGAIN, 6.0)
{
	this->plugin = plugin;
	set_precision(0.1);
}

int DenoiseFFTLevel::handle_event()
{
	plugin->config.level = get_value();
	plugin->send_configure_change();
	return 1;
}

DenoiseFFTCollecting::DenoiseFFTCollecting(DenoiseFFTEffect *plugin, int x, int y)
 : BC_CheckBox(x, y, plugin->config.collecting, "Collect noise")
{
	this->plugin = plugin;
}

int DenoiseFFTCollecting::handle_event()
{
	plugin->config.collecting = get_value();
	plugin->send_configure_change();
	return 1;
}

// DenoiseFFTSize::DenoiseFFTSize(DenoiseFFTEffect *plugin, int x, int y)
//  : BC_TextBox(x, y, 100, 1, plugin->config.window_size)
// {
// 	this->plugin = plugin;
// }
// int DenoiseFFTSize::handle_event()
// {
// 	plugin->config.window_size = atol(get_text());
// 	plugin->send_configure_change();
// 	return 1;
// }



DenoiseFFTWindow::DenoiseFFTWindow(DenoiseFFTEffect *plugin, int x, int y)
 : BC_Window(plugin->gui_string, 
 	x, 
	y, 
	220, 
	120, 
	220, 
	120,
	0, 
	0,
	1)
{
	this->plugin = plugin;
}

void DenoiseFFTWindow::create_objects()
{
	int x = 10, y = 10;

	add_subwindow(new BC_Title(x, y, "Denoise power:"));
	add_subwindow(level = new DenoiseFFTLevel(plugin, x + 130, y));
	y += 35;
	add_subwindow(collect = new DenoiseFFTCollecting(plugin, x, y));
// 	y += 35;
// 	add_subwindow(new BC_Title(x, y, "Window size:"));
// 	add_subwindow(size = new DenoiseFFTSize(plugin, x + 100, y));
	show_window();
	flush();
}

int DenoiseFFTWindow::close_event()
{
// Set result to 1 to indicate a client side close
	set_done(1);
	return 1;
}











PLUGIN_THREAD_OBJECT(DenoiseFFTEffect, DenoiseFFTThread, DenoiseFFTWindow)















DenoiseFFTEffect::DenoiseFFTEffect(PluginServer *server)
 : PluginAClient(server)
{
	reset();
	PLUGIN_CONSTRUCTOR_MACRO
}

DenoiseFFTEffect::~DenoiseFFTEffect()
{
	PLUGIN_DESTRUCTOR_MACRO
	if(reference) delete [] reference;
	if(remove_engine) delete remove_engine;
	if(collect_engine) delete collect_engine;
}

NEW_PICON_MACRO(DenoiseFFTEffect)

LOAD_CONFIGURATION_MACRO(DenoiseFFTEffect, DenoiseFFTConfig)

SHOW_GUI_MACRO(DenoiseFFTEffect, DenoiseFFTThread)

RAISE_WINDOW_MACRO(DenoiseFFTEffect)

SET_STRING_MACRO(DenoiseFFTEffect)


void DenoiseFFTEffect::reset()
{
	reference = 0;
	remove_engine = 0;
	collect_engine = 0;
	is_collecting = 0;
	prev_window_size = 0;
}

char* DenoiseFFTEffect::plugin_title()
{
	return "DenoiseFFT";
}


int DenoiseFFTEffect::is_realtime()
{
	return 1;
}



void DenoiseFFTEffect::read_data(KeyFrame *keyframe)
{
	FileXML input;
	input.set_shared_string(keyframe->data, strlen(keyframe->data));

	int result = 0;
	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("DENOISEFFT"))
			{
				config.window_size = input.tag.get_property("WINDOW_SIZE", config.window_size);
				config.level = input.tag.get_property("LEVEL", config.level);
				config.collecting = input.tag.get_property("COLLECTING", config.collecting);
			}
		}
	}
}

void DenoiseFFTEffect::save_data(KeyFrame *keyframe)
{
	FileXML output;
	output.set_shared_string(keyframe->data, MESSAGESIZE);

	output.tag.set_title("DENOISEFFT");
	output.tag.set_property("WINDOW_SIZE", config.window_size);
	output.tag.set_property("LEVEL", config.level);
	output.tag.set_property("COLLECTING", config.collecting);
	output.append_tag();
	output.append_newline();

	output.terminate_string();
}

int DenoiseFFTEffect::load_defaults()
{
	defaults = new Defaults(BCASTDIR "denoisefft.rc");
	defaults->load();

	config.level = defaults->get("LEVEL", config.level);
	config.window_size = defaults->get("WINDOW_SIZE", config.window_size);
	config.collecting = defaults->get("COLLECTING", config.collecting);
	return 0;
}

int DenoiseFFTEffect::save_defaults()
{
	char string[BCTEXTLEN];

	defaults->update("LEVEL", config.level);
	defaults->update("WINDOW_SIZE", config.window_size);
	defaults->update("COLLECTING", config.collecting);
	defaults->save();

	return 0;
}

void DenoiseFFTEffect::update_gui()
{
	if(thread)
	{
		load_configuration();
		thread->window->lock_window();
		thread->window->level->update(config.level);
		thread->window->collect->update(config.collecting);
//		thread->window->size->update((int64_t)config.window_size);
		thread->window->unlock_window();
	}
}

int DenoiseFFTEffect::process_realtime(int64_t size, double *input_ptr, double *output_ptr)
{
	load_configuration();

	if(prev_window_size != config.window_size)
	{
		if(reference) delete [] reference;
		if(collect_engine) delete collect_engine;
		if(remove_engine) delete remove_engine;
		reference = 0;
		collect_engine = 0;
		remove_engine = 0;
		prev_window_size = config.window_size;
	}

	if(config.collecting)
	{
// Create reference
		if(!reference)
		{
			reference = new double[config.window_size / 2];
		}

		if(!is_collecting)
			bzero(reference, sizeof(double) * config.window_size / 2);

		if(!collect_engine) collect_engine = new DenoiseFFTCollect(this);
		collect_engine->process_fifo(size, input_ptr, output_ptr);

		memcpy(output_ptr, input_ptr, sizeof(double) * size);
		is_collecting = 1;
	}
	else
	{
		is_collecting = 0;
// Hasn't collected yet.  Load from disk file
		if(!reference)
		{
			reference = new double[config.window_size / 2];
			FileSystem fs;
			char string[BCTEXTLEN];
			strcpy(string, REFERENCE_FILE);
			fs.complete_path(string);
			FILE *fd = fopen(string, "r");

			if(fd)
			{
				fread(reference, config.window_size / 2 * sizeof(double), 1, fd);
				fclose(fd);
			}
			else
			{
				bzero(reference, sizeof(double) * config.window_size / 2);
			}

		}

		if(!remove_engine) remove_engine = new DenoiseFFTRemove(this);
		remove_engine->process_fifo(size, input_ptr, output_ptr);
	}

	return 0;
}









DenoiseFFTRemove::DenoiseFFTRemove(DenoiseFFTEffect *plugin)
{
	this->plugin = plugin;
}

int DenoiseFFTRemove::signal_process()
{
	double level = DB::fromdb(plugin->config.level);
	for(int i = 0; i < window_size / 2; i++)
	{
		double result = sqrt(freq_real[i] * freq_real[i] + freq_imag[i] * freq_imag[i]);
		double angle = atan2(freq_imag[i], freq_real[i]);
		result -= plugin->reference[i] * level;
		if(result < 0) result = 0;
		freq_real[i] = result * cos(angle);
		freq_imag[i] = result * sin(angle);
	}
	symmetry(window_size, freq_real, freq_imag);
	return 0;
}





DenoiseFFTCollect::DenoiseFFTCollect(DenoiseFFTEffect *plugin)
{
	this->plugin = plugin;
}

int DenoiseFFTCollect::signal_process()
{
	for(int i = 0; i < window_size / 2; i++)
	{
// Moving average of peak so you don't need to stop, play, restart, play to 
// collect new data.
		double result = sqrt(freq_real[i] * freq_real[i] + freq_imag[i] * freq_imag[i]);
		double reference = plugin->reference[i];
		if(result < reference) 
			plugin->reference[i] = (result + reference * 4) / 5;
		else
			plugin->reference[i] = result;
	}

	FileSystem fs;
	char string[BCTEXTLEN];
	strcpy(string, REFERENCE_FILE);
	fs.complete_path(string);
	FILE *fd = fopen(string, "w");
	if(fd)
	{
		fwrite(plugin->reference, window_size / 2 * sizeof(double), 1, fd);
		fclose(fd);
	}
	else
		printf("DenoiseFFTCollect::signal_process: can't open %s for writing. %s.\n",
			REFERENCE_FILE, strerror(errno));
	return 0;
}

