#include "clip.h"
#include "colormodels.h"
#include "defaults.h"
#include "filexml.h"
#include "flip.h"
#include "flipwindow.h"
#include "language.h"
#include "picon_png.h"

#include <stdint.h>
#include <string.h>

REGISTER_PLUGIN(FlipMain)






FlipConfig::FlipConfig()
{
	flip_horizontal = 0;
	flip_vertical = 0;
}

void FlipConfig::copy_from(FlipConfig &that)
{
	flip_horizontal = that.flip_horizontal;
	flip_vertical = that.flip_vertical;
}

int FlipConfig::equivalent(FlipConfig &that)
{
	return flip_horizontal == that.flip_horizontal &&
		flip_vertical == that.flip_vertical;
}

void FlipConfig::interpolate(FlipConfig &prev, 
	FlipConfig &next, 
	long prev_frame, 
	long next_frame, 
	long current_frame)
{
	this->flip_horizontal = prev.flip_horizontal;
	this->flip_vertical = prev.flip_vertical;
}










FlipMain::FlipMain(PluginServer *server)
 : PluginVClient(server)
{
	PLUGIN_CONSTRUCTOR_MACRO
}

FlipMain::~FlipMain()
{
	PLUGIN_DESTRUCTOR_MACRO
}

char* FlipMain::plugin_title() { return N_("Flip"); }
int FlipMain::is_realtime() { return 1; }
	

#define SWAP_PIXELS(type, components, in, out) \
{ \
	type temp = in[0]; \
	in[0] = out[0]; \
	out[0] = temp; \
 \
 	temp = in[1]; \
	in[1] = out[1]; \
	out[1] = temp; \
 \
 	temp = in[2]; \
	in[2] = out[2]; \
	out[2] = temp; \
 \
	if(components == 4) \
	{ \
 		temp = in[3]; \
		in[3] = out[3]; \
		out[3] = temp; \
	} \
}

#define FLIP_MACRO(type, components) \
{ \
	type **input_rows, **output_rows; \
	type *input_row, *output_row; \
	input_rows = ((type**)input_ptr->get_rows()); \
	output_rows = ((type**)output_ptr->get_rows()); \
 \
	if(config.flip_vertical) \
	{ \
		for(i = 0, j = h - 1; i < h / 2; i++, j--) \
		{ \
			input_row = input_rows[i]; \
			output_row = output_rows[j]; \
			for(k = 0; k < w; k++) \
			{ \
				SWAP_PIXELS(type, components, output_row, input_row); \
				output_row += components; \
				input_row += components; \
			} \
		} \
	} \
 \
	if(config.flip_horizontal) \
	{ \
		for(i = 0; i < h; i++) \
		{ \
			input_row = input_rows[i]; \
			output_row = output_rows[i] + (w - 1) * components; \
			for(k = 0; k < w / 2; k++) \
			{ \
				SWAP_PIXELS(type, components, output_row, input_row); \
				input_row += components; \
				output_row -= components; \
			} \
		} \
	} \
}

int FlipMain::process_realtime(VFrame *input_ptr, VFrame *output_ptr)
{
	int i, j, k, l;
	int w = input_ptr->get_w();
	int h = input_ptr->get_h();
	int colormodel = input_ptr->get_color_model();

	load_configuration();
	switch(colormodel)
	{
		case BC_RGB888:
		case BC_YUV888:
			FLIP_MACRO(unsigned char, 3);
			break;
		case BC_RGB_FLOAT:
			FLIP_MACRO(float, 3);
			break;
		case BC_RGB161616:
		case BC_YUV161616:
			FLIP_MACRO(uint16_t, 3);
			break;
		case BC_RGBA8888:
		case BC_YUVA8888:
			FLIP_MACRO(unsigned char, 4);
			break;
		case BC_RGBA_FLOAT:
			FLIP_MACRO(float, 4);
			break;
		case BC_RGBA16161616:
		case BC_YUVA16161616:
			FLIP_MACRO(uint16_t, 4);
			break;
	}
	return 0;
}


SHOW_GUI_MACRO(FlipMain, FlipThread)

RAISE_WINDOW_MACRO(FlipMain)

SET_STRING_MACRO(FlipMain)

NEW_PICON_MACRO(FlipMain)

LOAD_CONFIGURATION_MACRO(FlipMain, FlipConfig)

void FlipMain::update_gui()
{
	if(thread)
	{
		load_configuration();
		thread->window->lock_window();
		thread->window->flip_vertical->update((int)config.flip_vertical);
		thread->window->flip_horizontal->update((int)config.flip_horizontal);
		thread->window->unlock_window();
	}
}

void FlipMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_string(keyframe->data, MESSAGESIZE);
	output.tag.set_title("FLIP");
	output.append_tag();
	if(config.flip_vertical)
	{
		output.tag.set_title("VERTICAL");
		output.append_tag();
	}

	if(config.flip_horizontal)
	{	
		output.tag.set_title("HORIZONTAL");
		output.append_tag();
	}
	output.terminate_string();
// data is now in *text
}

void FlipMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->data, strlen(keyframe->data));

	int result = 0;
	config.flip_vertical = config.flip_horizontal = 0;

	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("VERTICAL"))
			{
				config.flip_vertical = 1;
			}
			else
			if(input.tag.title_is("HORIZONTAL"))
			{
				config.flip_horizontal = 1;
			}
		}
	}
}

int FlipMain::load_defaults()
{
	char directory[BCTEXTLEN], string[BCTEXTLEN];
// set the default directory
	sprintf(directory, "%sflip.rc", BCASTDIR);

// load the defaults
	defaults = new Defaults(directory);
	defaults->load();

	config.flip_horizontal = defaults->get("FLIP_HORIZONTAL", config.flip_horizontal);
	config.flip_vertical = defaults->get("FLIP_VERTICAL", config.flip_vertical);
	return 0;
}

int FlipMain::save_defaults()
{
	defaults->update("FLIP_HORIZONTAL", config.flip_horizontal);
	defaults->update("FLIP_VERTICAL", config.flip_vertical);
	defaults->save();
	return 0;
}
