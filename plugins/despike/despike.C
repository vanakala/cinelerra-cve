#include "clip.h"
#include "confirmsave.h"
#include "defaults.h"
#include "errorbox.h"
#include "filexml.h"
#include "picon_png.h"
#include "despike.h"
#include "despikewindow.h"

#include "vframe.h"

#include <string.h>


REGISTER_PLUGIN(Despike)



Despike::Despike(PluginServer *server)
 : PluginAClient(server)
{
	PLUGIN_CONSTRUCTOR_MACRO
	last_sample = 0;
}

Despike::~Despike()
{
	PLUGIN_DESTRUCTOR_MACRO
}

char* Despike::plugin_title() { return "Despike"; }
int Despike::is_realtime() { return 1; }

NEW_PICON_MACRO(Despike)


SHOW_GUI_MACRO(Despike, DespikeThread)
SET_STRING_MACRO(Despike)
RAISE_WINDOW_MACRO(Despike)

LOAD_CONFIGURATION_MACRO(Despike, DespikeConfig)


int Despike::process_realtime(long size, double *input_ptr, double *output_ptr)
{
	load_configuration();

	double threshold = db.fromdb(config.level);
	double change = db.fromdb(config.slope);

//printf("Despike::process_realtime 1\n");
	for(long i = 0; i < size; i++)
	{
		if(fabs(input_ptr[i]) > threshold || 
			fabs(input_ptr[i]) - fabs(last_sample) > change) 
		{
			output_ptr[i] = last_sample;
		}
		else
		{
			output_ptr[i] = input_ptr[i];
			last_sample = input_ptr[i];
		}
	}
//printf("Despike::process_realtime 2\n");

	return 0;
}


int Despike::load_defaults()
{
	char directory[1024];

// set the default directory
	sprintf(directory, "%sdespike.rc", get_defaultdir());
	
// load the defaults

	defaults = new Defaults(directory);

	defaults->load();

	config.level = defaults->get("LEVEL", (double)0);
	config.slope = defaults->get("SLOPE", (double)0);

	return 0;
}

int Despike::save_defaults()
{
	defaults->update("LEVEL", config.level);
	defaults->update("SLOPE", config.slope);
	defaults->save();
	return 0;
}


void Despike::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause xml file to store data directly in text
	output.set_shared_string(keyframe->data, MESSAGESIZE);

	output.tag.set_title("DESPIKE");
	output.tag.set_property("LEVEL", config.level);
	output.tag.set_property("SLOPE", config.slope);
	output.append_tag();
	output.append_newline();
	output.terminate_string();
}

void Despike::read_data(KeyFrame *keyframe)
{
	FileXML input;
// cause xml file to read directly from text
	input.set_shared_string(keyframe->data, strlen(keyframe->data));
	int result = 0;

	result = input.read_tag();

	if(!result)
	{
		if(input.tag.title_is("DESPIKE"))
		{
			config.level = input.tag.get_property("LEVEL", config.level);
			config.slope = input.tag.get_property("SLOPE", config.slope);
		}
	}
}

void Despike::update_gui()
{
	if(thread)
	{
		load_configuration();
		thread->window->lock_window();
		thread->window->level->update(config.level);
		thread->window->slope->update(config.slope);
		thread->window->unlock_window();
	}
}
















DespikeConfig::DespikeConfig()
{
	level = 0;
	slope = 0;
}

int DespikeConfig::equivalent(DespikeConfig &that)
{
	return EQUIV(level, that.level) && 
		EQUIV(slope, that.slope);
}

void DespikeConfig::copy_from(DespikeConfig &that)
{
	level = that.level;
	slope = that.slope;
}

void DespikeConfig::interpolate(DespikeConfig &prev, 
		DespikeConfig &next, 
		long prev_frame, 
		long next_frame, 
		long current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);

	this->level = prev.level * prev_scale + next.level * next_scale;
	this->slope = prev.slope * prev_scale + next.slope * next_scale;
}






