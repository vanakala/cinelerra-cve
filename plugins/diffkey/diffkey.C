#ifndef DIFFKEY_H
#define DIFFKEY_H

#include "bcdisplayinfo.h"
#include "clip.h"
#include "defaults.h"
#include "filexml.h"
#include "guicast.h"
#include "language.h"
#include "loadbalance.h"
#include "plugincolors.h"
#include "pluginvclient.h"


#include <string.h>



class DiffKeyGUI;
class DiffKey;



class DiffKeyConfig
{
public:
	DiffKeyConfig();
	void copy_from(DiffKeyConfig &src);
	int equivalent(DiffKeyConfig &src);
	void interpolate(DiffKeyConfig &prev, 
		DiffKeyConfig &next, 
		int64_t prev_frame, 
		int64_t next_frame, 
		int64_t current_frame);

	float threshold;
	float slope;
	int do_value;
};


class DiffKeyThreshold : public BC_FSlider
{
public:
	DiffKeyThreshold(DiffKey *plugin, int x, int y);
	int handle_event();
	DiffKey *plugin;
};

class DiffKeySlope : public BC_FSlider
{
public:
	DiffKeySlope(DiffKey *plugin, int x, int y);
	int handle_event();
	DiffKey *plugin;
};

class DiffKeyDoValue : public BC_CheckBox
{
public:
	DiffKeyDoValue(DiffKey *plugin, int x, int y);
	int handle_event();
	DiffKey *plugin;
};



class DiffKeyGUI : public BC_Window
{
public:
	DiffKeyGUI(DiffKey *plugin, int x, int y);
	~DiffKeyGUI();


	void create_objects();
	int close_event();


	DiffKeyThreshold *threshold;
	DiffKeySlope *slope;
	DiffKeyDoValue *do_value;
	DiffKey *plugin;
};


PLUGIN_THREAD_HEADER(DiffKey, DiffKeyThread, DiffKeyGUI)



class DiffKeyEngine : public LoadServer
{
public:
	DiffKeyEngine(DiffKey *plugin);
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
	DiffKey *plugin;
};


class DiffKeyClient : public LoadClient
{
public:
	DiffKeyClient(DiffKeyEngine *engine);
	~DiffKeyClient();

	void process_package(LoadPackage *pkg);
	DiffKeyEngine *engine;
};

class DiffKeyPackage : public LoadPackage
{
public:
	DiffKeyPackage();
	int row1;
	int row2;
};



class DiffKey : public PluginVClient
{
public:
	DiffKey(PluginServer *server);
	~DiffKey();

	int process_buffer(VFrame **frame,
		int64_t start_position,
		double frame_rate);
	int is_realtime();
	int is_multichannel();
	int load_defaults();
	int save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void update_gui();



	PLUGIN_CLASS_MEMBERS(DiffKeyConfig, DiffKeyThread)

	DiffKeyEngine *engine;
	VFrame *top_frame;
	VFrame *bottom_frame;
};








REGISTER_PLUGIN(DiffKey)


DiffKeyConfig::DiffKeyConfig()
{
	threshold = 0.1;
	slope = 0;
	do_value = 0;
}

void DiffKeyConfig::copy_from(DiffKeyConfig &src)
{
	this->threshold = src.threshold;
	this->slope = src.slope;
	this->do_value = src.do_value;
}


int DiffKeyConfig::equivalent(DiffKeyConfig &src)
{
	return EQUIV(threshold, src.threshold) &&
		EQUIV(slope, src.slope) &&
		do_value == src.do_value;
}

void DiffKeyConfig::interpolate(DiffKeyConfig &prev, 
	DiffKeyConfig &next, 
	int64_t prev_frame, 
	int64_t next_frame, 
	int64_t current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);

	this->threshold = prev.threshold * prev_scale + next.threshold * next_scale;
	this->slope = prev.slope * prev_scale + next.slope * next_scale;
	this->do_value = prev.do_value;
}










DiffKeyThreshold::DiffKeyThreshold(DiffKey *plugin, int x, int y)
 : BC_FSlider(x, y, 0, 200, 200, 0, 100, plugin->config.threshold)
{
	this->plugin = plugin;
}

int DiffKeyThreshold::handle_event()
{
	plugin->config.threshold = get_value();
	plugin->send_configure_change();
	return 1;
}








DiffKeySlope::DiffKeySlope(DiffKey *plugin, int x, int y)
 : BC_FSlider(x, y, 0, 200, 200, 0, 100, plugin->config.slope)
{
	this->plugin = plugin;
}

int DiffKeySlope::handle_event()
{
	plugin->config.slope = get_value();
	plugin->send_configure_change();
	return 1;
}



DiffKeyDoValue::DiffKeyDoValue(DiffKey *plugin, int x, int y)
 : BC_CheckBox(x, y, plugin->config.do_value, _("Use Value"))
{
	this->plugin = plugin;
}

int DiffKeyDoValue::handle_event()
{
	plugin->config.do_value = get_value();
	plugin->send_configure_change();
	return 1;
}







DiffKeyGUI::DiffKeyGUI(DiffKey *plugin, int x, int y)
 : BC_Window(plugin->gui_string,
 	x,
	y,
	320,
	100,
	320,
	100,
	0,
	0,
	1)
{
	this->plugin = plugin;
}

DiffKeyGUI::~DiffKeyGUI()
{
}


void DiffKeyGUI::create_objects()
{
	int x = 10, y = 10, x2;
	BC_Title *title;
	add_subwindow(title = new BC_Title(x, y, "Threshold:"));
	x += title->get_w() + 10;
	add_subwindow(threshold = new DiffKeyThreshold(plugin, x, y));
	x = 10;
	y += threshold->get_h() + 10;
	add_subwindow(title = new BC_Title(x, y, "Slope:"));
	x += title->get_w() + 10;
	add_subwindow(slope = new DiffKeySlope(plugin, x, y));
	x = 10;
	y += slope->get_h() + 10;
	add_subwindow(do_value = new DiffKeyDoValue(plugin, x, y));



	show_window();
}

WINDOW_CLOSE_EVENT(DiffKeyGUI)


PLUGIN_THREAD_OBJECT(DiffKey, DiffKeyThread, DiffKeyGUI)



DiffKey::DiffKey(PluginServer *server)
 : PluginVClient(server)
{
	PLUGIN_CONSTRUCTOR_MACRO
	engine = 0;
}

DiffKey::~DiffKey()
{
	PLUGIN_DESTRUCTOR_MACRO
	delete engine;
}

SHOW_GUI_MACRO(DiffKey, DiffKeyThread)
RAISE_WINDOW_MACRO(DiffKey)
SET_STRING_MACRO(DiffKey)
#include "picon_png.h"
NEW_PICON_MACRO(DiffKey)
LOAD_CONFIGURATION_MACRO(DiffKey, DiffKeyConfig)

char* DiffKey::plugin_title() { return N_("Difference key"); }
int DiffKey::is_realtime() { return 1; }
int DiffKey::is_multichannel() { return 1; }

int DiffKey::load_defaults()
{
	char directory[BCTEXTLEN];
// set the default directory
	sprintf(directory, "%sdiffkey.rc", BCASTDIR);

// load the defaults
	defaults = new Defaults(directory);
	defaults->load();

	config.threshold = defaults->get("THRESHOLD", config.threshold);
	config.slope = defaults->get("SLOPE", config.slope);
	config.do_value = defaults->get("DO_VALUE", config.do_value);
	return 0;
}

int DiffKey::save_defaults()
{
	defaults->update("THRESHOLD", config.threshold);
	defaults->update("SLOPE", config.slope);
	defaults->update("DO_VALUE", config.do_value);
	defaults->save();
	return 0;
}

void DiffKey::save_data(KeyFrame *keyframe)
{
	FileXML output;
	output.set_shared_string(keyframe->data, MESSAGESIZE);
	output.tag.set_title("DIFFKEY");
	output.tag.set_property("THRESHOLD", config.threshold);
	output.tag.set_property("SLOPE", config.slope);
	output.tag.set_property("DO_VALUE", config.do_value);
	output.append_tag();
	output.terminate_string();
}

void DiffKey::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->data, strlen(keyframe->data));

	while(!input.read_tag())
	{
		if(input.tag.title_is("DIFFKEY"))
		{
			config.threshold = input.tag.get_property("THRESHOLD", config.threshold);
			config.slope = input.tag.get_property("SLOPE", config.slope);
			config.do_value = input.tag.get_property("DO_VALUE", config.do_value);
		}
	}
}

void DiffKey::update_gui()
{
	if(thread)
	{
		if(load_configuration())
		{
			thread->window->lock_window("DiffKey::update_gui");
			thread->window->threshold->update(config.threshold);
			thread->window->slope->update(config.slope);
			thread->window->do_value->update(config.do_value);
			thread->window->unlock_window();
		}
	}
}

int DiffKey::process_buffer(VFrame **frame,
	int64_t start_position,
	double frame_rate)
{
	load_configuration();

// Don't process if only 1 layer.
	if(get_total_buffers() < 2) 
	{
		read_frame(frame[0], 0, start_position, frame_rate);
		return 0;
	}

// Read frames from 2 layers
	read_frame(frame[0], 0, start_position, frame_rate);
	read_frame(frame[1], 1, start_position, frame_rate);

	top_frame = frame[0];
	bottom_frame = frame[1];

	if(!engine)
	{
		engine = new DiffKeyEngine(this);
	}

	engine->process_packages();

	return 0;
}







DiffKeyEngine::DiffKeyEngine(DiffKey *plugin) 
 : LoadServer(plugin->get_project_smp() + 1, plugin->get_project_smp() + 1)
{
	this->plugin = plugin;
}

void DiffKeyEngine::init_packages()
{
	int increment = plugin->top_frame->get_h() / get_total_packages() + 1;
	int y = 0;
	for(int i = 0; i < get_total_packages(); i++)
	{
		DiffKeyPackage *pkg = (DiffKeyPackage*)get_package(i);
		pkg->row1 = y;
		pkg->row2 = MIN(y + increment, plugin->top_frame->get_h()); 
		y  += increment;
	}
}

LoadClient* DiffKeyEngine::new_client()
{
	return new DiffKeyClient(this);
}

LoadPackage* DiffKeyEngine::new_package()
{
	return new DiffKeyPackage;
}











DiffKeyClient::DiffKeyClient(DiffKeyEngine *engine)
 : LoadClient(engine)
{
	this->engine = engine;
}

DiffKeyClient::~DiffKeyClient()
{
}

void DiffKeyClient::process_package(LoadPackage *ptr)
{
	DiffKeyPackage *pkg = (DiffKeyPackage*)ptr;
	DiffKey *plugin = engine->plugin;
	int w = plugin->top_frame->get_w();

#define RGB_TO_VALUE(r, g, b) \
((r) * R_TO_Y + (g) * G_TO_Y + (b) * B_TO_Y)

#define SQR(x) ((x) * (x))

#define DIFFKEY_MACRO(type, components, max, chroma_offset) \
{ \
	float threshold = plugin->config.threshold / 100; \
	float run = plugin->config.slope / 100; \
	float threshold_run = threshold + run; \
 \
	for(int i = pkg->row1; i < pkg->row2; i++) \
	{ \
		type *top_row = (type*)plugin->top_frame->get_rows()[i]; \
		type *bottom_row = (type*)plugin->bottom_frame->get_rows()[i]; \
 \
		for(int j = 0; j < w; j++) \
		{ \
			float a = 1.0; \
 \
/* Test for value in range */ \
			if(plugin->config.do_value) \
			{ \
				float top_value; \
				float bottom_value; \
 \
/* Convert pixel data into floating point value */ \
				if(chroma_offset) \
				{ \
					float top_r = (float)top_row[0] / max; \
					float bottom_r = (float)bottom_row[0] / max; \
					top_value = top_r; \
					bottom_value = bottom_r; \
				} \
				else \
				{ \
					float top_r = (float)top_row[0] / max; \
					float top_g = (float)top_row[1] / max; \
					float top_b = (float)top_row[2] / max; \
					top_g -= (float)chroma_offset / max; \
					top_b -= (float)chroma_offset / max; \
 \
					float bottom_r = (float)bottom_row[0] / max; \
					float bottom_g = (float)bottom_row[1] / max; \
					float bottom_b = (float)bottom_row[2] / max; \
					bottom_g -= (float)chroma_offset / max; \
					bottom_b -= (float)chroma_offset / max; \
 \
					top_value = RGB_TO_VALUE(top_r, top_g, top_b); \
					bottom_value = RGB_TO_VALUE(bottom_r, bottom_g, bottom_b); \
				} \
 \
 				float min_v = bottom_value - threshold; \
				float max_v = bottom_value + threshold; \
 \
/* Full transparency if in range */ \
				if(top_value >= min_v && top_value < max_v) \
				{ \
					a = 0; \
				} \
				else \
/* Phased out if below or above range */ \
				if(top_value < min_v) \
				{ \
					if(min_v - top_value < run) \
						a = (min_v - top_value) / run; \
				} \
				else \
				if(top_value - max_v < run) \
					a = (top_value - max_v) / run; \
			} \
			else \
/* Use color cube */ \
			{ \
				float top_r = (float)top_row[0] / max; \
				float top_g = (float)top_row[1] / max; \
				float top_b = (float)top_row[2] / max; \
				top_g -= (float)chroma_offset / max; \
				top_b -= (float)chroma_offset / max; \
 \
				float bottom_r = (float)bottom_row[0] / max; \
				float bottom_g = (float)bottom_row[1] / max; \
				float bottom_b = (float)bottom_row[2] / max; \
				bottom_g -= (float)chroma_offset / max; \
				bottom_b -= (float)chroma_offset / max; \
 \
/* Convert pixel values to RGB float */ \
 				if(chroma_offset) \
				{ \
					float y = bottom_r; \
					float u = bottom_g; \
					float v = bottom_b; \
					YUV::yuv_to_rgb_f(bottom_r, \
						bottom_g, \
						bottom_b, \
						y, \
						u, \
						v); \
					y = top_r; \
					u = top_g; \
					v = top_b; \
					YUV::yuv_to_rgb_f(top_r, \
						top_g, \
						top_b, \
						y, \
						u, \
						v); \
				} \
 \
				float difference = sqrt(SQR(top_r - bottom_r) +  \
					SQR(top_g - bottom_g) + \
					SQR(top_b - bottom_b)); \
 \
				if(difference < threshold) \
				{ \
					a = 0; \
				} \
				else \
				if(difference < threshold_run) \
				{ \
					a = (difference - threshold) / run; \
				} \
			} \
 \
/* multiply alpha */ \
			if(components == 4) \
			{ \
				top_row[3] = MIN((type)(a * max), top_row[3]); \
			} \
			else \
			{ \
				top_row[0] = (type)(a * top_row[0]); \
				top_row[1] = (type)(a * (top_row[1] - chroma_offset) + chroma_offset); \
				top_row[2] = (type)(a * (top_row[2] - chroma_offset) + chroma_offset); \
			} \
 \
			top_row += components; \
			bottom_row += components; \
		} \
	} \
}



	switch(plugin->top_frame->get_color_model())
	{
		case BC_RGB_FLOAT:
			DIFFKEY_MACRO(float, 3, 1.0, 0);
			break;
		case BC_RGBA_FLOAT:
			DIFFKEY_MACRO(float, 4, 1.0, 0);
			break;
		case BC_RGB888:
			DIFFKEY_MACRO(unsigned char, 3, 0xff, 0);
			break;
		case BC_RGBA8888:
			DIFFKEY_MACRO(unsigned char, 4, 0xff, 0);
			break;
		case BC_YUV888:
			DIFFKEY_MACRO(unsigned char, 3, 0xff, 0x80);
			break;
		case BC_YUVA8888:
			DIFFKEY_MACRO(unsigned char, 4, 0xff, 0x80);
			break;
	}



}




DiffKeyPackage::DiffKeyPackage()
 : LoadPackage()
{
}






#endif
