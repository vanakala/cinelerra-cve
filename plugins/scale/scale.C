#include "clip.h"
#include "filexml.h"
#include "picon_png.h"
#include "scale.h"
#include "scalewin.h"

#include <string.h>

REGISTER_PLUGIN(ScaleMain)



ScaleConfig::ScaleConfig()
{
	w = 1;
	h = 1;
	constrain = 0;
}

void ScaleConfig::copy_from(ScaleConfig &src)
{
	w = src.w;
	h = src.h;
	constrain = src.constrain;
}
int ScaleConfig::equivalent(ScaleConfig &src)
{
	return EQUIV(w, src.w) && 
		EQUIV(h, src.h) && 
		constrain == src.constrain;
}

void ScaleConfig::interpolate(ScaleConfig &prev, 
	ScaleConfig &next, 
	long prev_frame, 
	long next_frame, 
	long current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);

	this->w = prev.w * prev_scale + next.w * next_scale;
	this->h = prev.h * prev_scale + next.h * next_scale;
	this->constrain = prev.constrain;
}








ScaleMain::ScaleMain(PluginServer *server)
 : PluginVClient(server)
{
	temp_frame = 0;
	overlayer = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

ScaleMain::~ScaleMain()
{
	PLUGIN_DESTRUCTOR_MACRO

	if(temp_frame) delete temp_frame;
	temp_frame = 0;
	if(overlayer) delete overlayer;
	overlayer = 0;
}

char* ScaleMain::plugin_title() { return "Scale"; }
int ScaleMain::is_realtime() { return 1; }

NEW_PICON_MACRO(ScaleMain)

int ScaleMain::load_defaults()
{
	char directory[1024], string[1024];
// set the default directory
	sprintf(directory, "%sscale.rc", BCASTDIR);

// load the defaults
	defaults = new Defaults(directory);
	defaults->load();

	config.w = defaults->get("WIDTH", config.w);
	config.h = defaults->get("HEIGHT", config.h);
	config.constrain = defaults->get("CONSTRAIN", config.constrain);
//printf("ScaleMain::load_defaults %f %f\n",config.w  , config.h);
}

int ScaleMain::save_defaults()
{
	defaults->update("WIDTH", config.w);
	defaults->update("HEIGHT", config.h);
	defaults->update("CONSTRAIN", config.constrain);
	defaults->save();
}

LOAD_CONFIGURATION_MACRO(ScaleMain, ScaleConfig)


void ScaleMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_string(keyframe->data, MESSAGESIZE);

// Store data
	output.tag.set_title("SCALE");
	output.tag.set_property("WIDTH", config.w);
	output.tag.set_property("HEIGHT", config.h);
	output.append_tag();

	if(config.constrain)
	{
		output.tag.set_title("CONSTRAIN");
		output.append_tag();
	}
	output.terminate_string();
// data is now in *text
}

void ScaleMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->data, strlen(keyframe->data));

	int result = 0;
	config.constrain = 0;

	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("SCALE"))
			{
				config.w = input.tag.get_property("WIDTH", config.w);
				config.h = input.tag.get_property("HEIGHT", config.h);
			}
			else
			if(input.tag.title_is("CONSTRAIN"))
			{
				config.constrain = 1;
			}
		}
	}
}








int ScaleMain::process_realtime(VFrame *input_ptr, VFrame *output_ptr)
{
	VFrame *input, *output;
	
	input = input_ptr;
	output = output_ptr;

	load_configuration();

//printf("ScaleMain::process_realtime 1 %p\n", input);
	if(input->get_rows()[0] == output->get_rows()[0])
	{
		if(!temp_frame) 
			temp_frame = new VFrame(0, 
				input_ptr->get_w(), 
				input_ptr->get_h(),
				input->get_color_model());
		temp_frame->copy_from(input);
		input = temp_frame;
	}
//printf("ScaleMain::process_realtime 2 %p\n", input);

	if(!overlayer)
	{
		overlayer = new OverlayFrame(smp + 1);
	}


	if(config.w == 1 && config.h == 1)
	{
// No scaling
		if(input->get_rows()[0] != output->get_rows()[0])
		{
			output->copy_from(input);
		}
	}
	else
	{
// Perform scaling
		float center_x, center_y;
		float in_x1, in_x2, in_y1, in_y2, out_x1, out_x2, out_y1, out_y2;

		center_x = (float)input_ptr->get_w() / 2;
		center_y = (float)input_ptr->get_h() / 2;
		in_x1 = 0;
		in_x2 = input_ptr->get_w();
		in_y1 = 0;
		in_y2 = input_ptr->get_h();
		out_x1 = (float)center_x - (float)input_ptr->get_w() * config.w / 2;
		out_x2 = (float)center_x + (float)input_ptr->get_w() * config.w / 2;
		out_y1 = (float)center_y - (float)input_ptr->get_h() * config.h / 2;
		out_y2 = (float)center_y + (float)input_ptr->get_h() * config.h / 2;


//printf("ScaleMain::process_realtime %f = %d / 2\n", center_x, input_ptr->get_w());
//printf("ScaleMain::process_realtime %f = %f + %d * %f / 2\n", 
//	out_x1, center_x, input_ptr->get_w(), config.w);

		if(out_x1 < 0)
		{
			in_x1 += -out_x1 / config.w;
			out_x1 = 0;
		}

		if(out_x2 > input_ptr->get_w())
		{
			in_x2 -= (out_x2 - input_ptr->get_w()) / config.w;
			out_x2 = input_ptr->get_w();
		}

		if(out_y1 < 0)
		{
			in_y1 += -out_y1 / config.h;
			out_y1 = 0;
		}

		if(out_y2 > input_ptr->get_h())
		{
			in_y2 -= (out_y2 - input_ptr->get_h()) / config.h;
			out_y2 = input_ptr->get_h();
		}

		output->clear_frame();

// printf("ScaleMain::process_realtime 3 output=%p input=%p config.w=%f config.h=%f"
// 	"%f %f %f %f -> %f %f %f %f\n", 
// 	output,
// 	input,
// 	config.w, 
// 	config.h,
// 	in_x1, 
// 	in_y1, 
// 	in_x2, 
// 	in_y2,
// 	out_x1, 
// 	out_y1, 
// 	out_x2, 
// 	out_y2);
		overlayer->overlay(output, 
			input,
			in_x1, 
			in_y1, 
			in_x2, 
			in_y2,
			out_x1, 
			out_y1, 
			out_x2, 
			out_y2, 
			1,
			TRANSFER_REPLACE,
			get_interpolation_type());

	}
}



SHOW_GUI_MACRO(ScaleMain, ScaleThread)
RAISE_WINDOW_MACRO(ScaleMain)
SET_STRING_MACRO(ScaleMain)

void ScaleMain::update_gui()
{
	if(thread) 
	{
		load_configuration();
		thread->window->lock_window();
		thread->window->width->update(config.w);
		thread->window->height->update(config.h);
		thread->window->constrain->update(config.constrain);
		thread->window->unlock_window();
	}
}



