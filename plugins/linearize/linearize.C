#include "clip.h"
#include "filexml.h"
#include "linearize.h"
#include "defaults.h"
#include "language.h"
#include "picon_png.h"
#include "../colors/plugincolors.h"
#include "workarounds.h"

#include <stdio.h>
#include <string.h>

#define SQR(a) ((a) * (a))

REGISTER_PLUGIN(LinearizeMain)



LinearizeConfig::LinearizeConfig()
{
	max = 1;
	gamma = 0.6;
	automatic = 1;
}

int LinearizeConfig::equivalent(LinearizeConfig &that)
{
	return (EQUIV(max, that.max) && 
		EQUIV(gamma, that.gamma) &&
		automatic == that.automatic);
}

void LinearizeConfig::copy_from(LinearizeConfig &that)
{
	max = that.max;
	gamma = that.gamma;
	automatic = that.automatic;
}

void LinearizeConfig::interpolate(LinearizeConfig &prev, 
	LinearizeConfig &next, 
	int64_t prev_frame, 
	int64_t next_frame, 
	int64_t current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);

	this->max = prev.max * prev_scale + next.max * next_scale;
	this->gamma = prev.gamma * prev_scale + next.gamma * next_scale;
	this->automatic = prev.automatic;
}








LinearizePackage::LinearizePackage()
 : LoadPackage()
{
	start = end = 0;
}










LinearizeUnit::LinearizeUnit(LinearizeMain *plugin)
{
	this->plugin = plugin;
}
	
void LinearizeUnit::process_package(LoadPackage *package)
{
	LinearizePackage *pkg = (LinearizePackage*)package;
	LinearizeEngine *engine = (LinearizeEngine*)get_server();
	VFrame *data = engine->data;
	int w = data->get_w();
	float r, g, b, y, u, v;

// The same algorithm used by dcraw
	if(engine->operation == LinearizeEngine::HISTOGRAM)
	{
#define HISTOGRAM_HEAD(type) \
		for(int i = pkg->start; i < pkg->end; i++) \
		{ \
			type *row = (type*)data->get_rows()[i]; \
			for(int j = 0; j < w; j++) \
			{

#define HISTOGRAM_TAIL(components) \
				int slot; \
				slot = (int)(r * HISTOGRAM_SIZE); \
				accum[CLIP(slot, 0, HISTOGRAM_SIZE - 1)]++; \
				slot = (int)(g * HISTOGRAM_SIZE); \
				accum[CLIP(slot, 0, HISTOGRAM_SIZE - 1)]++; \
				slot = (int)(b * HISTOGRAM_SIZE); \
				accum[CLIP(slot, 0, HISTOGRAM_SIZE - 1)]++; \
				row += components; \
			} \
		}


		switch(data->get_color_model())
		{
			case BC_RGB888:
				HISTOGRAM_HEAD(unsigned char)
				r = (float)row[0] / 0xff;
				g = (float)row[1] / 0xff;
				b = (float)row[2] / 0xff;
				HISTOGRAM_TAIL(3)
				break;
			case BC_RGBA8888:
				HISTOGRAM_HEAD(unsigned char)
				r = (float)row[0] / 0xff;
				g = (float)row[1] / 0xff;
				b = (float)row[2] / 0xff;
				HISTOGRAM_TAIL(4)
				break;
			case BC_RGB_FLOAT:
				HISTOGRAM_HEAD(float)
				r = row[0];
				g = row[1];
				b = row[2];
				HISTOGRAM_TAIL(3)
				break;
			case BC_RGBA_FLOAT:
				HISTOGRAM_HEAD(float)
				r = row[0];
				g = row[1];
				b = row[2];
				HISTOGRAM_TAIL(4)
				break;
			case BC_YUV888:
				HISTOGRAM_HEAD(unsigned char)
				y = row[0];
				u = row[1];
				v = row[2];
				y /= 0xff;
				u = (float)((u - 0x80) / 0xff);
				v = (float)((v - 0x80) / 0xff);
				YUV::yuv_to_rgb_f(r, g, b, y, u, v);
				HISTOGRAM_TAIL(3)
				break;
			case BC_YUVA8888:
				HISTOGRAM_HEAD(unsigned char)
				y = (float)row[0] / 0xff;
				u = (float)row[1] / 0xff;
				v = (float)row[2] / 0xff;
				YUV::yuv_to_rgb_f(r, g, b, y, u, v);
				HISTOGRAM_TAIL(4)
				break;
		}
	}
	else
	{
		float max = plugin->config.max;
		float scale = 1.0 / max;
		float gamma = plugin->config.gamma - 1.0;

#define LINEARIZE_HEAD(type) \
		for(int i = pkg->start; i < pkg->end; i++) \
		{ \
			type *row = (type*)data->get_rows()[i]; \
			for(int j = 0; j < w; j++) \
			{

#define LINEARIZE_MID \
				r = r * scale * powf(r * 2 / max, gamma); \
				g = g * scale * powf(g * 2 / max, gamma); \
				b = b * scale * powf(b * 2 / max, gamma); \

#define LINEARIZE_TAIL(components) \
				row += components; \
			} \
		}


		switch(data->get_color_model())
		{
			case BC_RGB888:
				LINEARIZE_HEAD(unsigned char)
				r = (float)row[0] / 0xff;
				g = (float)row[1] / 0xff;
				b = (float)row[2] / 0xff;
				LINEARIZE_MID
				row[0] = (int)CLIP(r * 0xff, 0, 0xff);
				row[1] = (int)CLIP(g * 0xff, 0, 0xff);
				row[2] = (int)CLIP(b * 0xff, 0, 0xff);
				LINEARIZE_TAIL(3)
				break;
			case BC_RGBA8888:
				LINEARIZE_HEAD(unsigned char)
				r = (float)row[0] / 0xff;
				g = (float)row[1] / 0xff;
				b = (float)row[2] / 0xff;
				LINEARIZE_MID
				row[0] = (int)CLIP(r * 0xff, 0, 0xff);
				row[1] = (int)CLIP(g * 0xff, 0, 0xff);
				row[2] = (int)CLIP(b * 0xff, 0, 0xff);
				LINEARIZE_TAIL(4)
				break;
			case BC_RGB_FLOAT:
				LINEARIZE_HEAD(float)
				r = row[0];
				g = row[1];
				b = row[2];
				LINEARIZE_MID
				row[0] = r;
				row[1] = g;
				row[2] = b;
				LINEARIZE_TAIL(3)
				break;
			case BC_RGBA_FLOAT:
				LINEARIZE_HEAD(float)
				r = row[0];
				g = row[1];
				b = row[2];
				LINEARIZE_MID
				row[0] = r;
				row[1] = g;
				row[2] = b;
				LINEARIZE_TAIL(4)
				break;
			case BC_YUV888:
				LINEARIZE_HEAD(unsigned char)
				y = row[0];
				u = row[1];
				v = row[2];
				y /= 0xff;
				u = (float)((u - 0x80) / 0xff);
				v = (float)((v - 0x80) / 0xff);
				YUV::yuv_to_rgb_f(r, g, b, y, u, v);
				LINEARIZE_MID
				YUV::rgb_to_yuv_f(r, g, b, y, u, v);
				y *= 0xff;
				u = u * 0xff + 0x80;
				v = v * 0xff + 0x80;
				row[0] = (int)CLIP(y, 0, 0xff);
				row[1] = (int)CLIP(u, 0, 0xff);
				row[2] = (int)CLIP(v, 0, 0xff);
				LINEARIZE_TAIL(3)
				break;
			case BC_YUVA8888:
				LINEARIZE_HEAD(unsigned char)
				y = row[0];
				u = row[1];
				v = row[2];
				y /= 0xff;
				u = (float)((u - 0x80) / 0xff);
				v = (float)((v - 0x80) / 0xff);
				YUV::yuv_to_rgb_f(r, g, b, y, u, v);
				LINEARIZE_MID
				YUV::rgb_to_yuv_f(r, g, b, y, u, v);
				y *= 0xff;
				u = u * 0xff + 0x80;
				v = v * 0xff + 0x80;
				row[0] = (int)CLIP(y, 0, 0xff);
				row[1] = (int)CLIP(u, 0, 0xff);
				row[2] = (int)CLIP(v, 0, 0xff);
				LINEARIZE_TAIL(4)
				break;
		}
	}
}











LinearizeEngine::LinearizeEngine(LinearizeMain *plugin)
 : LoadServer(plugin->get_project_smp() + 1, 
 	plugin->get_project_smp() + 1)
{
	this->plugin = plugin;
}

void LinearizeEngine::init_packages()
{
	for(int i = 0; i < get_total_packages(); i++)
	{
		LinearizePackage *package = (LinearizePackage*)get_package(i);
		package->start = data->get_h() * i / get_total_packages();
		package->end = data->get_h() * (i + 1) / get_total_packages();
	}

// Initialize clients here in case some don't get run.
	for(int i = 0; i < get_total_clients(); i++)
	{
		LinearizeUnit *unit = (LinearizeUnit*)get_client(i);
		bzero(unit->accum, sizeof(int) * HISTOGRAM_SIZE);
	}
	bzero(accum, sizeof(int) * HISTOGRAM_SIZE);
}

LoadClient* LinearizeEngine::new_client()
{
	return new LinearizeUnit(plugin);
}

LoadPackage* LinearizeEngine::new_package()
{
	return new LinearizePackage;
}

void LinearizeEngine::process_packages(int operation, VFrame *data)
{
	this->data = data;
	this->operation = operation;
	LoadServer::process_packages();
	for(int i = 0; i < get_total_clients(); i++)
	{
		LinearizeUnit *unit = (LinearizeUnit*)get_client(i);
		for(int j = 0; j < HISTOGRAM_SIZE; j++)
		{
			accum[j] += unit->accum[j];
		}
	}
}
















LinearizeMain::LinearizeMain(PluginServer *server)
 : PluginVClient(server)
{
	engine = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

LinearizeMain::~LinearizeMain()
{
	PLUGIN_DESTRUCTOR_MACRO

	delete engine;
}

char* LinearizeMain::plugin_title() { return N_("Linearize"); }
int LinearizeMain::is_realtime() { return 1; }





NEW_PICON_MACRO(LinearizeMain)
LOAD_CONFIGURATION_MACRO(LinearizeMain, LinearizeConfig)
SHOW_GUI_MACRO(LinearizeMain, LinearizeThread)
RAISE_WINDOW_MACRO(LinearizeMain)
SET_STRING_MACRO(LinearizeMain)





int LinearizeMain::process_realtime(VFrame *input_ptr, VFrame *output_ptr)
{
	this->frame = input_ptr;
	load_configuration();


	if(config.automatic)
	{
		calculate_max(input_ptr);
		send_render_gui(this);
	}
	else
		send_render_gui(this);

	if(!engine) engine = new LinearizeEngine(this);
	engine->process_packages(LinearizeEngine::APPLY, input_ptr);
	return 0;
}

void LinearizeMain::calculate_max(VFrame *frame)
{
	if(!engine) engine = new LinearizeEngine(this);
	engine->process_packages(LinearizeEngine::HISTOGRAM, frame);
	int total_pixels = frame->get_w() * frame->get_h() * 3;
	int max_fraction = (int)((int64_t)total_pixels * 99 / 100);
	int current = 0;
	config.max = 1;
	for(int i = 0; i < HISTOGRAM_SIZE; i++)
	{
		current += engine->accum[i];
		if(current > max_fraction)
		{
			config.max = (float)i / HISTOGRAM_SIZE;
			break;
		}
	}
}


void LinearizeMain::update_gui()
{
	if(thread)
	{
		if(load_configuration())
		{
			thread->window->lock_window("LinearizeMain::update_gui");
			thread->window->update();
			thread->window->unlock_window();
		}
	}
}

void LinearizeMain::render_gui(void *data)
{
	LinearizeMain *ptr = (LinearizeMain*)data;
	config.max = ptr->config.max;

	if(!engine) engine = new LinearizeEngine(this);
	if(ptr->engine && ptr->config.automatic)
	{
		memcpy(engine->accum, 
			ptr->engine->accum, 
			sizeof(int) * HISTOGRAM_SIZE);
		thread->window->lock_window("LinearizeMain::render_gui");
		thread->window->update();
		thread->window->unlock_window();
	}
	else
	{
		engine->process_packages(LinearizeEngine::HISTOGRAM, 
			ptr->frame);
		thread->window->lock_window("LinearizeMain::render_gui");
		thread->window->update_histogram();
		thread->window->unlock_window();
	}


}

int LinearizeMain::load_defaults()
{
	char directory[1024], string[1024];
// set the default directory
	sprintf(directory, "%slinearize.rc", BCASTDIR);

// load the defaults
	defaults = new Defaults(directory);
	defaults->load();

	config.max = defaults->get("MAX", config.max);
	config.gamma = defaults->get("GAMMA", config.gamma);
	config.automatic = defaults->get("AUTOMATIC", config.automatic);
	return 0;
}

int LinearizeMain::save_defaults()
{
	defaults->update("MAX", config.max);
	defaults->update("GAMMA", config.gamma);
	defaults->update("AUTOMATIC", config.automatic);
	defaults->save();
	return 0;
}

void LinearizeMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_string(keyframe->data, MESSAGESIZE);
	output.tag.set_title("LINEARIZE");
	output.tag.set_property("MAX", config.max);
	output.tag.set_property("GAMMA", config.gamma);
	output.tag.set_property("AUTOMATIC",  config.automatic);
	output.append_tag();
	output.terminate_string();
}

void LinearizeMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->data, strlen(keyframe->data));

	int result = 0;

	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("LINEARIZE"))
			{
				config.max = input.tag.get_property("MAX", config.max);
				config.gamma = input.tag.get_property("GAMMA", config.gamma);
				config.automatic = input.tag.get_property("AUTOMATIC", config.automatic);
			}
		}
	}
}











