#include "clip.h"
#include "defaults.h"
#include "filexml.h"
#include "keyframe.h"
#include "picon_png.h"
#include "timeavg.h"
#include "timeavgwindow.h"
#include "vframe.h"

#include <libintl.h>
#define _(String) gettext(String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)




#include <stdint.h>
#include <string.h>




REGISTER_PLUGIN(TimeAvgMain)





TimeAvgConfig::TimeAvgConfig()
{
	frames = 1;
}



TimeAvgMain::TimeAvgMain(PluginServer *server)
 : PluginVClient(server)
{
	PLUGIN_CONSTRUCTOR_MACRO
	accumulation = 0;
	history = 0;
	history_size = 0;
}

TimeAvgMain::~TimeAvgMain()
{
//printf("TimeAvgMain::~TimeAvgMain 1\n");
	PLUGIN_DESTRUCTOR_MACRO

	if(accumulation) delete [] accumulation;
	if(history)
	{
		for(int i = 0; i < config.frames; i++)
			delete history[i];
		delete [] history;
	}
}

char* TimeAvgMain::plugin_title() { return _("Time Average"); }
int TimeAvgMain::is_realtime() { return 1; }


NEW_PICON_MACRO(TimeAvgMain)

SHOW_GUI_MACRO(TimeAvgMain, TimeAvgThread)

SET_STRING_MACRO(TimeAvgMain)

RAISE_WINDOW_MACRO(TimeAvgMain);



int TimeAvgMain::process_realtime(VFrame *input, VFrame *output)
{
	int h = input->get_h();
	int w = input->get_w();
	int color_model = input->get_color_model();

	load_configuration();

	if(!accumulation)
	{
		accumulation = new int[w * h * cmodel_components(color_model)];
		bzero(accumulation, sizeof(int) * w * h * cmodel_components(color_model));
	}

	if(history)
	{
		if(config.frames != history_size)
		{
			VFrame **history2;
			history2 = new VFrame*[config.frames];

// Copy existing frames over
			int i, j;
			for(i = 0, j = 0; i < config.frames && j < history_size; i++, j++)
				history2[i] = history[j];

// Delete extra previous frames
			for( ; j < history_size; j++)
				delete history[j];
			delete [] history;

// Create new frames
			for( ; i < config.frames; i++)
				history2[i] = new VFrame(0, w, h, color_model);

			history = history2;
			history_size = config.frames;

for(i = 0; i < config.frames; i++)
	bzero(history[i]->get_data(), history[i]->get_data_size());

bzero(accumulation, sizeof(int) * w * h * cmodel_components(color_model));
		}
	}
	else
	{
		history = new VFrame*[config.frames];
		for(int i = 0; i < config.frames; i++)
			history[i] = new VFrame(0, w, h, color_model);
		history_size = config.frames;
	}




// Cycle history
	VFrame *temp = history[0];
	for(int i = 0; i < history_size - 1; i++)
		history[i] = history[i + 1];
	history[history_size - 1] = temp;



// Subtract oldest history.  Add input.  Divide by total frames.
#define AVERAGE_MACRO(type, components, max) \
{ \
	for(int i = 0; i < h; i++) \
	{ \
		int *accum_row = accumulation + i * w * components; \
		type *out_row = (type*)output->get_rows()[i]; \
		type *in_row = (type*)input->get_rows()[i]; \
		type *subtract_row = (type*)history[history_size - 1]->get_rows()[i]; \
 \
		for(int j = 0; j < w * components; j++) \
		{ \
			accum_row[j] -= subtract_row[j]; \
			accum_row[j] += in_row[j]; \
			subtract_row[j] = in_row[j]; \
			out_row[j] = accum_row[j] / history_size; \
		} \
	} \
}







	switch(input->get_color_model())
	{
		case BC_RGB888:
		case BC_YUV888:
			AVERAGE_MACRO(unsigned char, 3, 0xff);
			break;

		case BC_RGBA8888:
		case BC_YUVA8888:
			AVERAGE_MACRO(unsigned char, 4, 0xff);
			break;

		case BC_RGB161616:
		case BC_YUV161616:
			AVERAGE_MACRO(uint16_t, 3, 0xffff);
			break;

		case BC_RGBA16161616:
		case BC_YUVA16161616:
			AVERAGE_MACRO(uint16_t, 4, 0xffff);
			break;
	}

//printf("TimeAvgMain::process_realtime 2\n");
	return 0;
}



int TimeAvgMain::load_defaults()
{
	char directory[BCTEXTLEN], string[BCTEXTLEN];
// set the default directory
	sprintf(directory, "%sbrightness.rc", BCASTDIR);

// load the defaults
	defaults = new Defaults(directory);
	defaults->load();

	config.frames = defaults->get("FRAMES", config.frames);
	return 0;
}

int TimeAvgMain::save_defaults()
{
	defaults->update("FRAMES", config.frames);
	defaults->save();
	return 0;
}

void TimeAvgMain::load_configuration()
{
	KeyFrame *prev_keyframe;
//printf("BlurMain::load_configuration 1\n");

	prev_keyframe = get_prev_keyframe(get_source_position());
	read_data(prev_keyframe);
}

void TimeAvgMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_string(keyframe->data, MESSAGESIZE);
	output.tag.set_title("TIME_AVERAGE");
	output.tag.set_property("FRAMES", config.frames);
	output.append_tag();
	output.terminate_string();
}

void TimeAvgMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->data, strlen(keyframe->data));

	int result = 0;

	while(!input.read_tag())
	{
		if(input.tag.title_is("TIME_AVERAGE"))
		{
			config.frames = input.tag.get_property("FRAMES", config.frames);
		}
	}
}


void TimeAvgMain::update_gui()
{
	if(thread) 
	{
		load_configuration();
		thread->window->lock_window();
		thread->window->total_frames->update(config.frames);
		thread->window->unlock_window();
	}
}
