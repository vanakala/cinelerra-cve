#include "clip.h"
#include "filexml.h"
#include "brightness.h"
#include "defaults.h"
#include "picon_png.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <libintl.h>
#define _(String) gettext(String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)

#define SQR(a) ((a) * (a))

REGISTER_PLUGIN(BrightnessMain)



BrightnessConfig::BrightnessConfig()
{
	brightness = 0;
	contrast = 0;
	luma = 1;
}

int BrightnessConfig::equivalent(BrightnessConfig &that)
{
	return (brightness == that.brightness && 
		contrast == that.contrast &&
		luma == that.luma);
}

void BrightnessConfig::copy_from(BrightnessConfig &that)
{
	brightness = that.brightness;
	contrast = that.contrast;
	luma = that.luma;
}

void BrightnessConfig::interpolate(BrightnessConfig &prev, 
	BrightnessConfig &next, 
	int64_t prev_frame, 
	int64_t next_frame, 
	int64_t current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);

	this->brightness = prev.brightness * prev_scale + next.brightness * next_scale;
	this->contrast = prev.contrast * prev_scale + next.contrast * next_scale;
	this->luma = (int)(prev.luma * prev_scale + next.luma * next_scale);
}









YUV BrightnessMain::yuv;

BrightnessMain::BrightnessMain(PluginServer *server)
 : PluginVClient(server)
{
    redo_buffers = 1;
	engine = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

BrightnessMain::~BrightnessMain()
{
	PLUGIN_DESTRUCTOR_MACRO
	if(engine) delete engine;
}

char* BrightnessMain::plugin_title() { return N_("Brightness/Contrast"); }
int BrightnessMain::is_realtime() { return 1; }

NEW_PICON_MACRO(BrightnessMain)	
SHOW_GUI_MACRO(BrightnessMain, BrightnessThread)
RAISE_WINDOW_MACRO(BrightnessMain)
SET_STRING_MACRO(BrightnessMain)
LOAD_CONFIGURATION_MACRO(BrightnessMain, BrightnessConfig)

int BrightnessMain::process_realtime(VFrame *input_ptr, VFrame *output_ptr)
{
	load_configuration();
	if(!engine) engine = new BrightnessEngine(this, PluginClient::smp + 1);

	this->input = input_ptr;
	this->output = output_ptr;

	if(!EQUIV(config.brightness, 0) || !EQUIV(config.contrast, 0))
	{
		engine->process_packages();
	}
	else
// Data never processed so copy if necessary
	if(input_ptr->get_rows()[0] != output_ptr->get_rows()[0])
	{
		output_ptr->copy_from(input_ptr);
	}

	return 0;
}



void BrightnessMain::update_gui()
{
	if(thread)
	{
		load_configuration();
		thread->window->lock_window();
		thread->window->brightness->update(config.brightness);
		thread->window->contrast->update(config.contrast);
		thread->window->luma->update(config.luma);
		thread->window->unlock_window();
	}
}

int BrightnessMain::load_defaults()
{
	char directory[1024], string[1024];
// set the default directory
	sprintf(directory, "%sbrightness.rc", BCASTDIR);

// load the defaults
	defaults = new Defaults(directory);
	defaults->load();

	config.brightness = defaults->get("BRIGHTNESS", config.brightness);
	config.contrast = defaults->get("CONTRAST", config.contrast);
	config.luma = defaults->get("LUMA", config.luma);
	return 0;
}

int BrightnessMain::save_defaults()
{
	defaults->update("BRIGHTNESS", config.brightness);
	defaults->update("CONTRAST", config.contrast);
	defaults->update("LUMA", config.luma);
	defaults->save();
	return 0;
}


void BrightnessMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_string(keyframe->data, MESSAGESIZE);
	output.tag.set_title("BRIGHTNESS");
	output.tag.set_property("BRIGHTNESS", config.brightness);
	output.tag.set_property("CONTRAST",  config.contrast);
	output.tag.set_property("LUMA",  config.luma);
//printf("BrightnessMain::save_data %d\n", config.luma);
	output.append_tag();
	output.terminate_string();
}

void BrightnessMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->data, strlen(keyframe->data));

	int result = 0;

	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("BRIGHTNESS"))
			{
				config.brightness = input.tag.get_property("BRIGHTNESS", config.brightness);
				config.contrast = input.tag.get_property("CONTRAST", config.contrast);
				config.luma = input.tag.get_property("LUMA", config.luma);
			}
		}
	}
}













BrightnessPackage::BrightnessPackage()
 : LoadPackage()
{
}




BrightnessUnit::BrightnessUnit(BrightnessEngine *server, BrightnessMain *plugin)
 : LoadClient(server)
{
	this->plugin = plugin;
}

BrightnessUnit::~BrightnessUnit()
{
}
	
void BrightnessUnit::process_package(LoadPackage *package)
{
	BrightnessPackage *pkg = (BrightnessPackage*)package;


	VFrame *output = plugin->output;
	VFrame *input = plugin->input;
	




#define DO_BRIGHTNESS(max, type, components, is_yuv) \
{ \
	type **input_rows = (type**)input->get_rows(); \
	type **output_rows = (type**)output->get_rows(); \
	int row1 = pkg->row1; \
	int row2 = pkg->row2; \
	int width = output->get_w(); \
	int r, g, b; \
 \
	if(!EQUIV(plugin->config.brightness, 0)) \
	{ \
		int offset = (int)(plugin->config.brightness / 100 * max); \
/*printf("DO_BRIGHTNESS offset=%d\n", offset);*/ \
 \
		for(int i = row1; i < row2; i++) \
		{ \
			type *input_row = input_rows[i]; \
			type *output_row = output_rows[i]; \
 \
			for(int j = 0; j < width; j++) \
			{ \
				r = input_row[j * components] + offset; \
 \
 				if(!is_yuv) \
				{ \
					g = input_row[j * components + 1] + offset; \
					b = input_row[j * components + 2] + offset; \
				} \
 \
				CLAMP(r, 0, max); \
				if(!is_yuv) \
				{ \
					CLAMP(g, 0, max); \
					CLAMP(b, 0, max); \
				} \
 \
				output_row[j * components] = r; \
 \
 				if(!is_yuv) \
				{ \
					output_row[j * components + 1] = g; \
					output_row[j * components + 2] = b; \
				} \
				else \
				{ \
					output_row[j * components + 1] = input_row[j * components + 1]; \
					output_row[j * components + 2] = input_row[j * components + 2]; \
				} \
 \
 				if(components == 4)  \
					output_row[j * components + 3] = input_row[j * components + 3]; \
			} \
		} \
 \
/* Data to be processed is now in the output buffer */ \
		input_rows = output_rows; \
	} \
 \
	if(!EQUIV(plugin->config.contrast, 0)) \
	{ \
		float contrast = (plugin->config.contrast < 0) ?  \
			(plugin->config.contrast + 100) / 100 :  \
			(plugin->config.contrast + 25) / 25; \
/*printf("DO_BRIGHTNESS contrast=%f\n", contrast);*/ \
 \
		int scalar = (int)(contrast * 0x100); \
		int offset = (max << 8) / 2 - max * scalar / 2; \
		int y, u, v; \
 \
		for(int i = row1; i < row2; i++) \
		{ \
			type *input_row = input_rows[i]; \
			type *output_row = output_rows[i]; \
 \
 			if(plugin->config.luma) \
			{ \
				for(int j = 0; j < width; j++) \
				{ \
					if(is_yuv) \
					{ \
						y = input_row[j * components]; \
					} \
					else \
					{ \
						r = input_row[j * components]; \
						g = input_row[j * components + 1]; \
						b = input_row[j * components + 2]; \
						if(max == 0xff) \
						{ \
							BrightnessMain::yuv.rgb_to_yuv_8( \
								r,  \
								g,  \
								b,  \
								y,  \
								u,  \
								v); \
						} \
						else \
						{ \
							BrightnessMain::yuv.rgb_to_yuv_16( \
								r,  \
								g,  \
								b,  \
								y,  \
								u,  \
								v); \
						} \
	 \
					} \
	 \
					y = (y * scalar + offset) >> 8; \
					CLAMP(y, 0, max); \
	 \
	 \
 					if(is_yuv) \
					{ \
						output_row[j * components] = y; \
						output_row[j * components + 1] = input_row[j * components + 1]; \
						output_row[j * components + 2] = input_row[j * components + 2]; \
					} \
					else \
					{ \
						if(max == 0xff) \
						{ \
							BrightnessMain::yuv.yuv_to_rgb_8( \
								r,  \
								g,  \
								b,  \
								y,  \
								u,  \
								v); \
						} \
						else \
						{ \
							BrightnessMain::yuv.yuv_to_rgb_16( \
								r,  \
								g,  \
								b,  \
								y,  \
								u,  \
								v); \
						} \
						input_row[j * components] = r; \
						input_row[j * components + 1] = g; \
						input_row[j * components + 2] = b; \
					} \
	 \
 					if(components == 4)  \
						output_row[j * components + 3] = input_row[j * components + 3]; \
				} \
			} \
			else \
			{ \
				for(int j = 0; j < width; j++) \
				{ \
					r = input_row[j * components]; \
					g = input_row[j * components + 1]; \
					b = input_row[j * components + 2]; \
 \
					r = (r * scalar + offset) >> 8; \
					g = (g * scalar + offset) >> 8; \
					b = (b * scalar + offset) >> 8; \
 \
					CLAMP(r, 0, max); \
					CLAMP(g, 0, max); \
					CLAMP(b, 0, max); \
 \
					output_row[j * components] = r; \
					output_row[j * components + 1] = g; \
					output_row[j * components + 2] = b; \
 \
 					if(components == 4)  \
						output_row[j * components + 3] = input_row[j * components + 3]; \
				} \
			} \
		} \
	} \
}


	switch(input->get_color_model())
	{
		case BC_RGB888:
			DO_BRIGHTNESS(0xff, unsigned char, 3, 0)
			break;

		case BC_YUV888:
			DO_BRIGHTNESS(0xff, unsigned char, 3, 1)
			break;

		case BC_RGBA8888:
			DO_BRIGHTNESS(0xff, unsigned char, 4, 0)
			break;

		case BC_YUVA8888:
			DO_BRIGHTNESS(0xff, unsigned char, 4, 1)
			break;

		case BC_RGB161616:
			DO_BRIGHTNESS(0xffff, uint16_t, 3, 0)
			break;

		case BC_YUV161616:
			DO_BRIGHTNESS(0xffff, uint16_t, 3, 1)
			break;

		case BC_RGBA16161616:
			DO_BRIGHTNESS(0xffff, uint16_t, 4, 0)
			break;

		case BC_YUVA16161616:
			DO_BRIGHTNESS(0xffff, uint16_t, 4, 1)
			break;
	}









}






BrightnessEngine::BrightnessEngine(BrightnessMain *plugin, int cpus)
 : LoadServer(cpus, cpus)
{
	this->plugin = plugin;
}

BrightnessEngine::~BrightnessEngine()
{
}


void BrightnessEngine::init_packages()
{
	for(int i = 0; i < LoadServer::total_packages; i++)
	{
		BrightnessPackage *package = (BrightnessPackage*)LoadServer::packages[i];
		package->row1 = (int)(plugin->input->get_h() / 
			LoadServer::total_packages * 
			i);
		package->row2 = package->row1 + 
			(int)(plugin->input->get_h() / 
			LoadServer::total_packages);

		if(i >= LoadServer::total_packages - 1)
			package->row2 = plugin->input->get_h();
	}
}

LoadClient* BrightnessEngine::new_client()
{
	return new BrightnessUnit(this, plugin);
}

LoadPackage* BrightnessEngine::new_package()
{
	return new BrightnessPackage;
}





