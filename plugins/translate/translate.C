#include "clip.h"
#include "filexml.h"
#include "picon_png.h"
#include "translate.h"
#include "translatewin.h"

#include <string.h>

#include <libintl.h>
#define _(String) gettext(String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)


REGISTER_PLUGIN(TranslateMain)

TranslateConfig::TranslateConfig()
{
	in_x = 0;
	in_y = 0;
	in_w = 720;
	in_h = 480;
	out_x = 0;
	out_y = 0;
	out_w = 720;
	out_h = 480;
}

int TranslateConfig::equivalent(TranslateConfig &that)
{
	return EQUIV(in_x, that.in_x) && 
		EQUIV(in_y, that.in_y) && 
		EQUIV(in_w, that.in_w) && 
		EQUIV(in_h, that.in_h) &&
		EQUIV(out_x, that.out_x) && 
		EQUIV(out_y, that.out_y) && 
		EQUIV(out_w, that.out_w) &&
		EQUIV(out_h, that.out_h);
}

void TranslateConfig::copy_from(TranslateConfig &that)
{
	in_x = that.in_x;
	in_y = that.in_y;
	in_w = that.in_w;
	in_h = that.in_h;
	out_x = that.out_x;
	out_y = that.out_y;
	out_w = that.out_w;
	out_h = that.out_h;
}

void TranslateConfig::interpolate(TranslateConfig &prev, 
	TranslateConfig &next, 
	int64_t prev_frame, 
	int64_t next_frame, 
	int64_t current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);

	this->in_x = prev.in_x * prev_scale + next.in_x * next_scale;
	this->in_y = prev.in_y * prev_scale + next.in_y * next_scale;
	this->in_w = prev.in_w * prev_scale + next.in_w * next_scale;
	this->in_h = prev.in_h * prev_scale + next.in_h * next_scale;
	this->out_x = prev.out_x * prev_scale + next.out_x * next_scale;
	this->out_y = prev.out_y * prev_scale + next.out_y * next_scale;
	this->out_w = prev.out_w * prev_scale + next.out_w * next_scale;
	this->out_h = prev.out_h * prev_scale + next.out_h * next_scale;
}








TranslateMain::TranslateMain(PluginServer *server)
 : PluginVClient(server)
{
	temp_frame = 0;
	overlayer = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

TranslateMain::~TranslateMain()
{
	PLUGIN_DESTRUCTOR_MACRO

	if(temp_frame) delete temp_frame;
	temp_frame = 0;
	if(overlayer) delete overlayer;
	overlayer = 0;
}

char* TranslateMain::plugin_title() { return N_("Translate"); }
int TranslateMain::is_realtime() { return 1; }

NEW_PICON_MACRO(TranslateMain)

int TranslateMain::load_defaults()
{
	char directory[1024], string[1024];
// set the default directory
	sprintf(directory, "%stranslate.rc", BCASTDIR);

// load the defaults
	defaults = new Defaults(directory);
	defaults->load();


	config.in_x = defaults->get("IN_X", config.in_x);
	config.in_y = defaults->get("IN_Y", config.in_y);
	config.in_w = defaults->get("IN_W", config.in_w);
	config.in_h = defaults->get("IN_H", config.in_h);
	config.out_x = defaults->get("OUT_X", config.out_x);
	config.out_y = defaults->get("OUT_Y", config.out_y);
	config.out_w = defaults->get("OUT_W", config.out_w);
	config.out_h = defaults->get("OUT_H", config.out_h);
}

int TranslateMain::save_defaults()
{
	defaults->update("IN_X", config.in_x);
	defaults->update("IN_Y", config.in_y);
	defaults->update("IN_W", config.in_w);
	defaults->update("IN_H", config.in_h);
	defaults->update("OUT_X", config.out_x);
	defaults->update("OUT_Y", config.out_y);
	defaults->update("OUT_W", config.out_w);
	defaults->update("OUT_H", config.out_h);
	defaults->save();
}

LOAD_CONFIGURATION_MACRO(TranslateMain, TranslateConfig)

void TranslateMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_string(keyframe->data, MESSAGESIZE);

// Store data
	output.tag.set_title("TRANSLATE");
	output.tag.set_property("IN_X", config.in_x);
	output.tag.set_property("IN_Y", config.in_y);
	output.tag.set_property("IN_W", config.in_w);
	output.tag.set_property("IN_H", config.in_h);
	output.tag.set_property("OUT_X", config.out_x);
	output.tag.set_property("OUT_Y", config.out_y);
	output.tag.set_property("OUT_W", config.out_w);
	output.tag.set_property("OUT_H", config.out_h);
	output.append_tag();

	output.terminate_string();
// data is now in *text
}

void TranslateMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->data, strlen(keyframe->data));

	int result = 0;

	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("TRANSLATE"))
			{
 				config.in_x = input.tag.get_property("IN_X", config.in_x);
				config.in_y = input.tag.get_property("IN_Y", config.in_y);
				config.in_w = input.tag.get_property("IN_W", config.in_w);
				config.in_h = input.tag.get_property("IN_H", config.in_h);
				config.out_x =	input.tag.get_property("OUT_X", config.out_x);
				config.out_y =	input.tag.get_property("OUT_Y", config.out_y);
				config.out_w =	input.tag.get_property("OUT_W", config.out_w);
				config.out_h =	input.tag.get_property("OUT_H", config.out_h);
			}
		}
	}
}








int TranslateMain::process_realtime(VFrame *input_ptr, VFrame *output_ptr)
{
	VFrame *input, *output;
	
	
	input = input_ptr;
	output = output_ptr;

	load_configuration();

//printf("TranslateMain::process_realtime 1 %p\n", input);
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
//printf("TranslateMain::process_realtime 2 %p\n", input);


	if(!overlayer)
	{
		overlayer = new OverlayFrame(smp + 1);
	}

	output->clear_frame();


// printf("TranslateMain::process_realtime 3 output=%p input=%p config.w=%f config.h=%f"
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
			config.in_x, 
			config.in_y, 
			config.in_x + config.in_w,
			config.in_y + config.in_h,
			config.out_x, 
			config.out_y, 
			config.out_x + config.out_w,
			config.out_y + config.out_h,
			1,
			TRANSFER_REPLACE,
			get_interpolation_type());

}




SHOW_GUI_MACRO(TranslateMain, TranslateThread)

RAISE_WINDOW_MACRO(TranslateMain)

SET_STRING_MACRO(TranslateMain)

void TranslateMain::update_gui()
{
	if(thread)
	{
		if(load_configuration())
		{
			thread->window->lock_window();
			thread->window->in_x->update(config.in_x);
			thread->window->in_y->update(config.in_y);
			thread->window->in_w->update(config.in_w);
			thread->window->in_h->update(config.in_h);
			thread->window->out_x->update(config.out_x);
			thread->window->out_y->update(config.out_y);
			thread->window->out_w->update(config.out_w);
			thread->window->out_h->update(config.out_h);
			thread->window->unlock_window();
		}
	}
}
