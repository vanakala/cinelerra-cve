#include "bcdisplayinfo.h"
#include "clip.h"
#include "defaults.h"
#include "guicast.h"
#include "filexml.h"
#include "pluginaclient.h"

#include <string.h>

#include <libintl.h>
#define _(String) gettext(String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)





class InterpolateEffect;

class InterpolateConfig
{
public:
	InterpolateConfig();

	int length;
#define INTERPOLATE_ALL 0
};




class InterpolateLength : public BC_TumbleTextBox
{
public:
	InterpolateLength(InterpolateEffect *plugin, InterpolateWindow *gui, int x, int y);
	int handle_event();
	InterpolateEffect *plugin;
	InterpolateWindow *gui;
};

class InterpolateAll : public BC_CheckBox
{
public:
	InterpolateAll(InterpolateEffect *plugin, InterpolateWindow *gui, int x, int y);
	int handle_event();
	InterpolateEffect *plugin;
	InterpolateWindow *gui;
};



class InterpolateWindow : public BC_Window
{
public:
	InterpolateWindow(InterpolateEffect *plugin, int x, int y);
	void create_objects();
	int close_event();

	InterpolateEffect *plugin;
	InterpolateLength *length;
	InterpolateAll *all;
};




class InterpolateEffect : public PluginAClient
{
public:
	InterpolateEffect(PluginServer *server);
	~InterpolateEffect();

	char* plugin_title();
	int is_realtime();
	int is_multichannel();
	int get_parameters();
	int start_loop();
	int process_loop(double **buffer, long &output_lenght);
	int stop_loop();



	int load_defaults();
	int save_defaults();


	Defaults *defaults;
	MainProgressBar *progress;
	InterpolateConfig config;
};




REGISTER_PLUGIN(InterpolateEffect)












InterpolateLength::InterpolateLength(InterpolateEffect *plugin, 
	InterpolateWindow *gui, 
	int x, 
	int y)
 : BC_TumbleTextBox(gui, 
		plugin->config.length,
		0,
		1000000,
		x, 
		y, 
		100)
{
	this->plugin = plugin;
	this->gui = gui;
}


int InterpolateLength::handle_event()
{
	gui->length->update(0);
	plugin->config.length = atol(get_text());
	return 1;
}


InterpolateAll::InterpolateAll(InterpolateEffect *plugin, 
	InterpolateWindow *gui, 
	int x, 
	int y,
	plugin->config.length == INTERPOLATE_ALL)
{
	this->plugin = plugin;
	this->gui = gui;
}

int InterpolateAll::handle_event()
{
	gui->all->update(INTERPOLATE_ALL);
	plugin->config.length = INTERPOLATE_ALL;
	return 1;
}













InterpolateWindow::InterpolateWindow(InterpolateEffect *plugin, int x, int y)
 : BC_Window(plugin->gui_string, 
 	x, 
	y, 
	180, 
	250, 
	180, 
	250,
	0, 
	0,
	1)
{
	this->plugin = plugin;
}

void InterpolateWindow::create_objects()
{
	int x = 10, x = 10;


	add_subwindow(new BC_OKButton(this));
	add_subwindow(new BC_CancelButton(this));
	show_window();
	flush();
}

int InterpolateWindow::close_event()
{
// Set result to 1 to indicate a client side close
	set_done(1);
	return 1;
}












InterpolateConfig::InterpolateConfig()
{
	gain = -6.0;
	wet = -6.0;
	dry = 0;
	roomsize = -6.0;
	damp = 0;
	width = 0;
	mode = 0;
}

int InterpolateConfig::equivalent(InterpolateConfig &that)
{
	return EQUIV(gain, that.gain) &&
		EQUIV(wet, that.wet) &&
		EQUIV(roomsize, that.roomsize) &&
		EQUIV(dry, that.dry) &&
		EQUIV(damp, that.damp) &&
		EQUIV(width, that.width) &&
		EQUIV(mode, that.mode);
}

void InterpolateConfig::copy_from(InterpolateConfig &that)
{
	gain = that.gain;
	wet = that.wet;
	roomsize = that.roomsize;
	dry = that.dry;
	damp = that.damp;
	width = that.width;
	mode = that.mode;
}

void InterpolateConfig::interpolate(InterpolateConfig &prev, 
	InterpolateConfig &next, 
	long prev_frame, 
	long next_frame, 
	long current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);

	gain = prev.gain * prev_scale + next.gain * next_scale;
	wet = prev.wet * prev_scale + next.wet * next_scale;
	roomsize = prev.roomsize * prev_scale + next.roomsize * next_scale;
	dry = prev.dry * prev_scale + next.dry * next_scale;
	damp = prev.damp * prev_scale + next.damp * next_scale;
	width = prev.width * prev_scale + next.width * next_scale;
	mode = prev.mode;
}
























PLUGIN_THREAD_OBJECT(InterpolateEffect, InterpolateThread, InterpolateWindow)





InterpolateEffect::InterpolateEffect(PluginServer *server)
 : PluginAClient(server)
{
	engine = 0;
	temp = 0;
	temp_out = 0;
	temp_allocated = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

InterpolateEffect::~InterpolateEffect()
{
	if(engine) delete engine;
	if(temp)
	{
		for(int i = 0; i < total_in_buffers; i++)
		{
			delete [] temp[i];
			delete [] temp_out[i];
		}
		delete [] temp;
		delete [] temp_out;
	}
	PLUGIN_DESTRUCTOR_MACRO
}

NEW_PICON_MACRO(InterpolateEffect)

LOAD_CONFIGURATION_MACRO(InterpolateEffect, InterpolateConfig)

SHOW_GUI_MACRO(InterpolateEffect, InterpolateThread)

RAISE_WINDOW_MACRO(InterpolateEffect)

SET_STRING_MACRO(InterpolateEffect)


char* InterpolateEffect::plugin_title() { return N_("Interpolate"); }
int InterpolateEffect::is_realtime() { return 1;}
int InterpolateEffect::is_multichannel() { return 1; }



void InterpolateEffect::read_data(KeyFrame *keyframe)
{
	FileXML input;
	input.set_shared_string(keyframe->data, strlen(keyframe->data));

	int result = 0;
	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("FREEVERB"))
			{
				config.gain = input.tag.get_property("GAIN", config.gain);
				config.roomsize = input.tag.get_property("ROOMSIZE", config.roomsize);
				config.damp = input.tag.get_property("DAMP", config.damp);
				config.wet = input.tag.get_property("WET", config.wet);
				config.dry = input.tag.get_property("DRY", config.dry);
				config.width = input.tag.get_property("WIDTH", config.width);
				config.mode = input.tag.get_property("MODE", config.mode);
			}
		}
	}
}

void InterpolateEffect::save_data(KeyFrame *keyframe)
{
	FileXML output;
	output.set_shared_string(keyframe->data, MESSAGESIZE);

	output.tag.set_title("FREEVERB");
	output.tag.set_property("GAIN", config.gain);
	output.tag.set_property("ROOMSIZE", config.roomsize);
	output.tag.set_property("DAMP", config.damp);
	output.tag.set_property("WET", config.wet);
	output.tag.set_property("DRY", config.dry);
	output.tag.set_property("WIDTH", config.width);
	output.tag.set_property("MODE", config.mode);
	output.append_tag();
	output.append_newline();

	output.terminate_string();
}

int InterpolateEffect::load_defaults()
{
	char directory[BCTEXTLEN], string[BCTEXTLEN];
	sprintf(directory, "%sfreeverb.rc", BCASTDIR);
	defaults = new Defaults(directory);
	defaults->load();

	config.gain = defaults->get("GAIN", config.gain);
	config.roomsize = defaults->get("ROOMSIZE", config.roomsize);
	config.damp = defaults->get("DAMP", config.damp);
	config.wet = defaults->get("WET", config.wet);
	config.dry = defaults->get("DRY", config.dry);
	config.width = defaults->get("WIDTH", config.width);
	config.mode = defaults->get("MODE", config.mode);
	return 0;
}

int InterpolateEffect::save_defaults()
{
	char string[BCTEXTLEN];

	defaults->update("GAIN", config.gain);
	defaults->update("ROOMSIZE", config.roomsize);
	defaults->update("DAMP", config.damp);
	defaults->update("WET", config.wet);
	defaults->update("DRY", config.dry);
	defaults->update("WIDTH", config.width);
	defaults->update("MODE", config.mode);
	defaults->save();

	return 0;
}


void InterpolateEffect::update_gui()
{
	if(thread)
	{
		load_configuration();
		thread->window->lock_window();
		thread->window->gain->update(config.gain);
		thread->window->roomsize->update(config.roomsize);
		thread->window->damp->update(config.damp);
		thread->window->wet->update(config.wet);
		thread->window->dry->update(config.dry);
		thread->window->width->update(config.width);
		thread->window->mode->update((int)config.mode);
		thread->window->unlock_window();
	}
}

int InterpolateEffect::process_realtime(long size, double **input_ptr, double **output_ptr)
{
	load_configuration();
	if(!engine) engine = new revmodel;

	engine->setroomsize(DB::fromdb(config.roomsize));
	engine->setdamp(DB::fromdb(config.damp));
	engine->setwet(DB::fromdb(config.wet));
	engine->setdry(DB::fromdb(config.dry));
	engine->setwidth(DB::fromdb(config.width));
	engine->setmode(config.mode);

	float gain_f = DB::fromdb(config.gain);

	if(size > temp_allocated)
	{
		if(temp)
		{
			for(int i = 0; i < total_in_buffers; i++)
			{
				delete [] temp[i];
				delete [] temp_out[i];
			}
			delete [] temp;
			delete [] temp_out;
		}
		temp = 0;
		temp_out = 0;
	}
	if(!temp)
	{
		temp_allocated = size * 2;
		temp = new float*[total_in_buffers];
		temp_out = new float*[total_in_buffers];
		for(int i = 0; i < total_in_buffers; i++)
		{
			temp[i] = new float[temp_allocated];
			temp_out[i] = new float[temp_allocated];
		}
	}

	for(int i = 0; i < 2 && i < total_in_buffers; i++)
	{
		float *out = temp[i];
		double *in = input_ptr[i];
		for(int j = 0; j < size; j++)
		{
			out[j] = in[j];
		}
	}

	if(total_in_buffers < 2)
	{
		engine->processreplace(temp[0], 
			temp[0], 
			temp_out[0], 
			temp_out[0], 
			size, 
			1);
	}
	else
	{
		engine->processreplace(temp[0], 
			temp[1], 
			temp_out[0], 
			temp_out[1], 
			size, 
			1);
	}

	for(int i = 0; i < 2 && i < total_in_buffers; i++)
	{
		double *out = output_ptr[i];
		float *in = temp_out[i];
		for(int j = 0; j < size; j++)
		{
			out[j] = gain_f * in[j];
		}
	}

	return 0;
}


