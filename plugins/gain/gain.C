#include "clip.h"
#include "confirmsave.h"
#include "defaults.h"
#include "errorbox.h"
#include "filexml.h"
#include "picon_png.h"
#include "gain.h"
#include "gainwindow.h"

#include "vframe.h"

#include <string.h>

#include <libintl.h>
#define _(String) gettext(String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)


REGISTER_PLUGIN(Gain)


GainConfig::GainConfig()
{
	level = 0.0;
}

int GainConfig::equivalent(GainConfig &that)
{
	return EQUIV(level, that.level);
}

void GainConfig::copy_from(GainConfig &that)
{
	this->level = that.level;
}

void GainConfig::interpolate(GainConfig &prev, 
	GainConfig &next, 
	int64_t prev_frame, 
	int64_t next_frame, 
	int64_t current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);
	level = prev.level * prev_scale + next.level * next_scale;
}









Gain::Gain(PluginServer *server)
 : PluginAClient(server)
{
	PLUGIN_CONSTRUCTOR_MACRO
}

Gain::~Gain()
{
	PLUGIN_DESTRUCTOR_MACRO
}

char* Gain::plugin_title() { return _("Gain"); }
int Gain::is_realtime() { return 1; }


SHOW_GUI_MACRO(Gain, GainThread)
SET_STRING_MACRO(Gain)
RAISE_WINDOW_MACRO(Gain)
NEW_PICON_MACRO(Gain)
LOAD_CONFIGURATION_MACRO(Gain, GainConfig)

int Gain::process_realtime(int64_t size, double *input_ptr, double *output_ptr)
{
	load_configuration();

	double gain = db.fromdb(config.level);

	for(int64_t i = 0; i < size; i++)
	{
		output_ptr[i] = input_ptr[i] * gain;
	}

	return 0;
}



int Gain::load_defaults()
{
	char directory[1024];

// set the default directory
	sprintf(directory, "%sgain.rc", get_defaultdir());
	
// load the defaults

	defaults = new Defaults(directory);

	defaults->load();

	config.level = defaults->get("LEVEL", config.level);

	return 0;
}

int Gain::save_defaults()
{
	defaults->update("LEVEL", config.level);
	defaults->save();
	return 0;
}

void Gain::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause xml file to store data directly in text
	output.set_shared_string(keyframe->data, MESSAGESIZE);

	output.tag.set_title("GAIN");
	output.tag.set_property("LEVEL", config.level);
	output.append_tag();
	output.append_newline();
	output.terminate_string();
}

void Gain::read_data(KeyFrame *keyframe)
{
	FileXML input;
// cause xml file to read directly from text
	input.set_shared_string(keyframe->data, strlen(keyframe->data));
	int result = 0;

	result = input.read_tag();

	if(!result)
	{
		if(input.tag.title_is("GAIN"))
		{
			config.level = input.tag.get_property("LEVEL", config.level);
		}
	}
}

void Gain::update_gui()
{
	if(thread)
	{
		load_configuration();
		thread->window->lock_window();
		thread->window->level->update(config.level);
		thread->window->unlock_window();
	}
}


