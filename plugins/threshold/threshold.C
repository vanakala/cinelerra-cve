#include "clip.h"
#include "defaults.h"
#include "filexml.h"
#include "histogramengine.h"
#include "language.h"
#include "plugincolors.h"
#include "threshold.h"
#include "thresholdwindow.h"
#include "vframe.h"

#include <string.h>


ThresholdConfig::ThresholdConfig()
{
	reset();
}

int ThresholdConfig::equivalent(ThresholdConfig &that)
{
	return EQUIV(min, that.min) &&
		EQUIV(max, that.max);
}

void ThresholdConfig::copy_from(ThresholdConfig &that)
{
	min = that.min;
	max = that.max;
}

void ThresholdConfig::interpolate(ThresholdConfig &prev,
	ThresholdConfig &next,
	int64_t prev_frame, 
	int64_t next_frame, 
	int64_t current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / 
		(next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / 
		(next_frame - prev_frame);

	min = prev.min * prev_scale + next.min * next_scale;
	max = prev.max * prev_scale + next.max * next_scale;
}

void ThresholdConfig::reset()
{
	min = 0.0;
	max = 1.0;
}

void ThresholdConfig::boundaries()
{
	CLAMP(min, HISTOGRAM_MIN, max);
	CLAMP(max, min, HISTOGRAM_MAX);
}








REGISTER_PLUGIN(ThresholdMain)

ThresholdMain::ThresholdMain(PluginServer *server)
 : PluginVClient(server)
{
	PLUGIN_CONSTRUCTOR_MACRO
	engine = 0;
	threshold_engine = 0;
}

ThresholdMain::~ThresholdMain()
{
	PLUGIN_DESTRUCTOR_MACRO
	delete engine;
	delete threshold_engine;
}

int ThresholdMain::is_realtime()
{
	return 1;
}

char* ThresholdMain::plugin_title() 
{ 
	return N_("Threshold"); 
}


#include "picon_png.h"
NEW_PICON_MACRO(ThresholdMain)

SHOW_GUI_MACRO(ThresholdMain, ThresholdThread)

SET_STRING_MACRO(ThresholdMain)

RAISE_WINDOW_MACRO(ThresholdMain)

LOAD_CONFIGURATION_MACRO(ThresholdMain, ThresholdConfig)







int ThresholdMain::process_buffer(VFrame *frame,
	int64_t start_position,
	double frame_rate)
{
	load_configuration();
	read_frame(frame,
		0,
		get_source_position(),
		get_framerate());
	send_render_gui(frame);

	if(!threshold_engine)
		threshold_engine = new ThresholdEngine(this);
	threshold_engine->process_packages(frame);
	
	return 0;
}

int ThresholdMain::load_defaults()
{
	char directory[BCTEXTLEN], string[BCTEXTLEN];
	sprintf(directory, "%sthreshold.rc", BCASTDIR);
	defaults = new Defaults(directory);
	defaults->load();
	config.min = defaults->get("MIN", config.min);
	config.max = defaults->get("MAX", config.max);
	config.boundaries();
	return 0;
}

int ThresholdMain::save_defaults()
{
	defaults->update("MIN", config.min);
	defaults->update("MAX", config.max);
	defaults->save();
}

void ThresholdMain::save_data(KeyFrame *keyframe)
{
	FileXML file;
	file.set_shared_string(keyframe->data, MESSAGESIZE);
	file.tag.set_title("THRESHOLD");
	file.tag.set_property("MIN", config.min);
	file.tag.set_property("MAX", config.max);
	file.append_tag();
	file.terminate_string();
}

void ThresholdMain::read_data(KeyFrame *keyframe)
{
	FileXML file;
	file.set_shared_string(keyframe->data, strlen(keyframe->data));
	int result = 0;
	while(!result)
	{
		result = file.read_tag();
		if(!result)
		{
			config.min = file.tag.get_property("MIN", config.min);
			config.max = file.tag.get_property("MAX", config.max);
		}
	}
	config.boundaries();
}

void ThresholdMain::update_gui()
{
	if(thread)
	{
		thread->window->lock_window("ThresholdMain::update_gui");
		if(load_configuration())
		{
			thread->window->min->update(config.min);
			thread->window->max->update(config.max);
		}
		thread->window->unlock_window();
	}
}

void ThresholdMain::render_gui(void *data)
{
	if(thread)
	{
		calculate_histogram((VFrame*)data);
		thread->window->lock_window("ThresholdMain::render_gui");
		thread->window->canvas->draw();
		thread->window->unlock_window();
	}
}

void ThresholdMain::calculate_histogram(VFrame *frame)
{
	if(!engine) engine = new HistogramEngine(get_project_smp() + 1,
		get_project_smp() + 1);
	engine->process_packages(frame);
}

























ThresholdPackage::ThresholdPackage()
 : LoadPackage()
{
	start = end = 0;
}











ThresholdUnit::ThresholdUnit(ThresholdEngine *server)
 : LoadClient(server)
{
	this->server = server;
}

void ThresholdUnit::process_package(LoadPackage *package)
{
	ThresholdPackage *pkg = (ThresholdPackage*)package;
	VFrame *data = server->data;
	int min = (int)(server->plugin->config.min * 0xffff);
	int max = (int)(server->plugin->config.max * 0xffff);
	int r, g, b, a, y, u, v;
	int w = server->data->get_w();
	int h = server->data->get_h();

#define THRESHOLD_HEAD(type) \
{ \
	for(int i = pkg->start; i < pkg->end; i++) \
	{ \
		type *in_row = (type*)data->get_rows()[i]; \
		type *out_row = in_row; \
		for(int j = 0; j < w; j++) \
		{

#define THRESHOLD_TAIL(components, r_on, g_on, b_on, a_on, r_off, g_off, b_off, a_off) \
			v = (r * 76 + g * 150 + b * 29) >> 8; \
			if(v >= min && v < max) \
			{ \
				*out_row++ = r_on; \
				*out_row++ = g_on; \
				*out_row++ = b_on; \
				if(components == 4) *out_row++ = a_on; \
			} \
			else \
			{ \
				*out_row++ = r_off; \
				*out_row++ = g_off; \
				*out_row++ = b_off; \
				if(components == 4) *out_row++ = a_off; \
			} \
			in_row += components; \
		} \
	} \
}


	switch(data->get_color_model())
	{
		case BC_RGB888:
			THRESHOLD_HEAD(unsigned char)
			r = (in_row[0] << 8) | in_row[0];
			g = (in_row[1] << 8) | in_row[1];
			b = (in_row[2] << 8) | in_row[2];
			THRESHOLD_TAIL(3, 0xff, 0xff, 0xff, 0xff, 0x0, 0x0, 0x0, 0x0);
			break;
		case BC_RGB_FLOAT:
			THRESHOLD_HEAD(float)
			r = (int)(in_row[0] * 0xffff);
			g = (int)(in_row[1] * 0xffff);
			b = (int)(in_row[2] * 0xffff);
			THRESHOLD_TAIL(3, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 0.0);
			break;
		case BC_RGBA8888:
			THRESHOLD_HEAD(unsigned char)
			r = (in_row[0] << 8) | in_row[0];
			g = (in_row[1] << 8) | in_row[1];
			b = (in_row[2] << 8) | in_row[2];
			THRESHOLD_TAIL(4, 0xff, 0xff, 0xff, 0xff, 0x0, 0x0, 0x0, 0x0);
			break;
		case BC_RGBA_FLOAT:
			THRESHOLD_HEAD(float)
			r = (int)(in_row[0] * 0xffff);
			g = (int)(in_row[1] * 0xffff);
			b = (int)(in_row[2] * 0xffff);
			THRESHOLD_TAIL(4, 1.0, 1.0, 1.0, 1.0, 0, 0, 0, 0);
			break;
		case BC_YUV888:
			THRESHOLD_HEAD(unsigned char)
			y = (in_row[0] << 8) | in_row[0];
			u = (in_row[1] << 8) | in_row[1];
			v = (in_row[2] << 8) | in_row[2];
			server->yuv->yuv_to_rgb_16(r, g, b, y, u, v);
			THRESHOLD_TAIL(3, 0xff, 0x80, 0x80, 0xff, 0x0, 0x80, 0x80, 0x0)
			break;
		case BC_YUVA8888:
			THRESHOLD_HEAD(unsigned char)
			y = (in_row[0] << 8) | in_row[0];
			u = (in_row[1] << 8) | in_row[1];
			v = (in_row[2] << 8) | in_row[2];
			server->yuv->yuv_to_rgb_16(r, g, b, y, u, v);
			THRESHOLD_TAIL(4, 0xff, 0x80, 0x80, 0xff, 0x0, 0x80, 0x80, 0x0)
			break;
		case BC_YUV161616:
			THRESHOLD_HEAD(uint16_t)
			y = in_row[0];
			u = in_row[1];
			v = in_row[2];
			server->yuv->yuv_to_rgb_16(r, g, b, y, u, v);
			THRESHOLD_TAIL(3, 0xffff, 0x8000, 0x8000, 0xffff, 0x0, 0x8000, 0x8000, 0x0)
			break;
		case BC_YUVA16161616:
			THRESHOLD_HEAD(uint16_t)
			y = in_row[0];
			u = in_row[1];
			v = in_row[2];
			server->yuv->yuv_to_rgb_16(r, g, b, y, u, v);
			THRESHOLD_TAIL(4, 0xffff, 0x8000, 0x8000, 0xffff, 0x0, 0x8000, 0x8000, 0x0)
			break;
	}
}











ThresholdEngine::ThresholdEngine(ThresholdMain *plugin)
 : LoadServer(plugin->get_project_smp() + 1,
 	plugin->get_project_smp() + 1)
{
	this->plugin = plugin;
	yuv = new YUV;
}

ThresholdEngine::~ThresholdEngine()
{
	delete yuv;
}

void ThresholdEngine::process_packages(VFrame *data)
{
	this->data = data;
	LoadServer::process_packages();
}

void ThresholdEngine::init_packages()
{
	int total_size = data->get_h();
	int package_size = (int)((float)total_size /
		get_total_packages() + 1);
	int start = 0;
	for(int i = 0; i < get_total_packages(); i++)
	{
		ThresholdPackage *package = (ThresholdPackage*)get_package(i);
		package->start = start;
		package->end = start + package_size;
		package->end = MIN(total_size, package->end);
		start = package->end;
	}
}

LoadClient* ThresholdEngine::new_client()
{
	return (LoadClient*)new ThresholdUnit(this);
}

LoadPackage* ThresholdEngine::new_package()
{
	return (LoadPackage*)new HistogramPackage;
}






