#include "bcdisplayinfo.h"
#include "chromakey.h"
#include "clip.h"
#include "defaults.h"
#include "filexml.h"
#include "guicast.h"
#include "keyframe.h"
#include "loadbalance.h"
#include "picon_png.h"
#include "plugincolors.h"
#include "pluginvclient.h"
#include "vframe.h"

#include <stdint.h>
#include <string.h>




ChromaKeyConfig::ChromaKeyConfig()
{
	hue = 0.0;
	threshold = 60.0;
	value = 0.1;
	use_value = 0;
	slope = 100;
}


void ChromaKeyConfig::copy_from(ChromaKeyConfig &src)
{
	hue = src.hue;
	threshold = src.threshold;
	value = src.value;
	use_value = src.use_value;
	slope = src.slope;
}

int ChromaKeyConfig::equivalent(ChromaKeyConfig &src)
{
	return (EQUIV(hue, src.hue) &&
		EQUIV(threshold, src.threshold) &&
		EQUIV(value, src.value) &&
		EQUIV(slope, src.slope) &&
		use_value == src.use_value);
}

void ChromaKeyConfig::interpolate(ChromaKeyConfig &prev, 
	ChromaKeyConfig &next, 
	int64_t prev_frame, 
	int64_t next_frame, 
	int64_t current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);

	this->hue = prev.hue * prev_scale + next.hue * next_scale;
	this->threshold = prev.threshold * prev_scale + next.threshold * next_scale;
	this->value = prev.value * prev_scale + next.value * next_scale;
	this->use_value = prev.use_value;
	this->slope = prev.slope * prev_scale + next.slope * next_scale;
}


ChromaKeyWindow::ChromaKeyWindow(ChromaKey *plugin, int x, int y)
 : BC_Window(plugin->gui_string, 
 	x, 
	y, 
	320, 
	230, 
	320, 
	230, 
	0, 
	0,
	1)
{
	this->plugin = plugin;
}

ChromaKeyWindow::~ChromaKeyWindow()
{
}

void ChromaKeyWindow::create_objects()
{
	int x = 10, y = 10, x1 = 100;

	for(int i = 0; i < 200; i++)
	{
		float r, g, b;
		HSV::hsv_to_rgb(r, g, b, i * 360 / 200, 1, 1);
		int color = (((int)(r * 0xff)) << 16) | (((int)(g * 0xff)) << 8)  | ((int)(b * 0xff));
		set_color(color);
		draw_line(x1 + i, y, x1 + i, y + 25);
	}
	y += 30;
	add_subwindow(new BC_Title(x, y, "Hue:"));
	add_subwindow(hue = new ChromaKeyHue(plugin, x1, y));
	y += 30;

	for(int i = 0; i < 200; i++)
	{
		int r = i * 0xff / 200;
		set_color((r << 16) | (r << 8) | r);
		draw_line(x1 + i, y, x1 + i, y + 25);
	}

	y += 30;
	add_subwindow(new BC_Title(x, y, "Value:"));
	add_subwindow(value = new ChromaKeyValue(plugin, x1, y));

	y += 30;
	add_subwindow(new BC_Title(x, y, "Slope:"));
	add_subwindow(slope = new ChromaKeySlope(plugin, x1, y));

	y += 30;
	add_subwindow(new BC_Title(x, y, "Threshold:"));
	add_subwindow(threshold = new ChromaKeyThreshold(plugin, x1, y));


	y += 30;
	add_subwindow(use_value = new ChromaKeyUseValue(plugin, x1, y));

	show_window();
	flush();
}

int ChromaKeyWindow::close_event()
{
	set_done(1);
	return 1;
}

ChromaKeyHue::ChromaKeyHue(ChromaKey *plugin, int x, int y)
 : BC_FSlider(x, 
			y,
			0,
			200, 
			200, 
			(float)0, 
			(float)360, 
			plugin->config.hue)
{
	this->plugin = plugin;
	set_precision(0.1);
}
int ChromaKeyHue::handle_event()
{
	plugin->config.hue = get_value();
	plugin->send_configure_change();
	return 1;
}

ChromaKeyThreshold::ChromaKeyThreshold(ChromaKey *plugin, int x, int y)
 : BC_FSlider(x, 
			y,
			0,
			200, 
			200, 
			(float)0, 
			(float)100, 
			plugin->config.threshold)
{
	this->plugin = plugin;
	set_precision(0.01);
}
int ChromaKeyThreshold::handle_event()
{
	plugin->config.threshold = get_value();
	plugin->send_configure_change();
	return 1;
}

ChromaKeyValue::ChromaKeyValue(ChromaKey *plugin, int x, int y)
 : BC_FSlider(x, 
			y,
			0,
			200, 
			200, 
			(float)0, 
			(float)100, 
			plugin->config.value)
{
	this->plugin = plugin;
	set_precision(0.01);
}
int ChromaKeyValue::handle_event()
{
	plugin->config.value = get_value();
	plugin->send_configure_change();
	return 1;
}



ChromaKeySlope::ChromaKeySlope(ChromaKey *plugin, int x, int y)
 : BC_FSlider(x, 
			y,
			0,
			200, 
			200, 
			(float)0, 
			(float)100, 
			plugin->config.slope)
{
	this->plugin = plugin;
	set_precision(0.01);
}
int ChromaKeySlope::handle_event()
{
	plugin->config.slope = get_value();
	plugin->send_configure_change();
	return 1;
}


ChromaKeyUseValue::ChromaKeyUseValue(ChromaKey *plugin, int x, int y)
 : BC_CheckBox(x, y, plugin->config.use_value, "Use value")
{
	this->plugin = plugin;
}
int ChromaKeyUseValue::handle_event()
{
	plugin->config.use_value = get_value();
	plugin->send_configure_change();
	return 1;
}









PLUGIN_THREAD_OBJECT(ChromaKey, ChromaKeyThread, ChromaKeyWindow)


ChromaKeyServer::ChromaKeyServer(ChromaKey *plugin)
 : LoadServer(plugin->PluginClient::smp + 1, plugin->PluginClient::smp + 1)
{
	this->plugin = plugin;
}
void ChromaKeyServer::init_packages()
{
	int increment = plugin->input->get_h() / get_total_packages() + 1;
	int y = 0;
	for(int i = 0; i < get_total_packages(); i++)
	{
		ChromaKeyPackage *pkg = (ChromaKeyPackage*)get_package(i);
		pkg->y1 = y;
		pkg->y2 = y + increment;
		y += increment;
		if(pkg->y2 > plugin->input->get_h())
		{
			y = pkg->y2 = plugin->input->get_h();
		}
		if(pkg->y1 > plugin->input->get_h())
		{
			y = pkg->y1 = plugin->input->get_h();
		}
	}
	
}
LoadClient* ChromaKeyServer::new_client()
{
	return new ChromaKeyUnit(plugin, this);
}
LoadPackage* ChromaKeyServer::new_package()
{
	return new ChromaKeyPackage();
}



ChromaKeyPackage::ChromaKeyPackage()
 : LoadPackage()
{
}

ChromaKeyUnit::ChromaKeyUnit(ChromaKey *plugin, ChromaKeyServer *server)
 : LoadClient(server)
{
	this->plugin = plugin;
}
void ChromaKeyUnit::process_package(LoadPackage *package)
{
	ChromaKeyPackage *pkg = (ChromaKeyPackage*)package;

	int w = plugin->input->get_w();
	float min_h = plugin->config.hue - plugin->config.threshold * 3.60 / 2;
	float max_h = plugin->config.hue + plugin->config.threshold * 3.60 / 2;
	float min_v = (plugin->config.value - plugin->config.threshold / 2) / 100;
	float max_v = (plugin->config.value + plugin->config.threshold / 2) / 100;
	int a;

	float run;
	if(plugin->config.use_value)
		run = (max_v - min_v) / 2 * (1 - plugin->config.slope / 100);
	else
		run = (max_h - min_h) / 2 * (1 - plugin->config.slope / 100);

// printf("ChromaKeyUnit::process_package %f %f %f %f\n", 
// 	min_h, max_h, min_v, max_v);

#define CHROMAKEY(type, max, components, use_yuv) \
{ \
	int chroma_offset; \
	if(use_yuv) \
		chroma_offset = max / 2 + 1; \
	else \
		chroma_offset = 0; \
	for(int i = pkg->y1; i < pkg->y2; i++) \
	{ \
		float h, s, va; \
		type *row = (type*)plugin->input->get_rows()[i]; \
 \
		for(int j = 0; j < w; j++) \
		{ \
			if(use_yuv) \
			{ \
				HSV::yuv_to_hsv(row[0],  \
					row[1],  \
					row[2],  \
					h,  \
					s,  \
					va,  \
					max); \
			} \
			else \
			{ \
				HSV::rgb_to_hsv((float)row[0] / max,  \
					(float)row[1] / max,  \
					(float)row[2] / max,  \
					h, s, va); \
			} \
 \
			if(plugin->config.use_value) \
			{ \
				if(va >= min_v && va <= max_v) \
				{ \
					if(va - min_v < max_v - va) \
					{ \
						if(va - min_v < run) \
							a = (int)(max - (va - min_v) / run * max); \
						else \
							a = 0; \
					} \
					else \
					{ \
						if(max_v - va < run) \
							a = (int)(max - (max_v - va) / run * max); \
						else \
							a = 0; \
					} \
 \
					if(components == 4) \
						row[3] = MIN(a, row[3]); \
					else \
					{ \
						row[0] = (a * row[0]) / max; \
						row[1] = (a * row[1] + (max - a) * chroma_offset) / max; \
						row[2] = (a * row[2] + (max - a) * chroma_offset) / max; \
					} \
				} \
			} \
			else \
			{ \
				if(h >= min_h && h <= max_h) \
				{ \
					if(h - min_h < max_h - h) \
					{ \
						if(h - min_h < run) \
							a = (int)(max - (h - min_h) / run * max); \
						else \
							a = 0; \
					} \
					else \
					{ \
						if(max_h - h < run) \
							a = (int)(max - (max_h - h) / run * max); \
						else \
							a = 0; \
					} \
 \
					if(components == 4) \
						row[3] = MIN(a, row[3]); \
					else \
					{ \
						row[0] = a * row[0] / max; \
						row[1] = (a * row[1] + (max - a) * chroma_offset) / max; \
						row[2] = (a * row[2] + (max - a) * chroma_offset) / max; \
					} \
				} \
			} \
 \
			row += components; \
		} \
	} \
}




	switch(plugin->input->get_color_model())
	{
		case BC_RGB888:
			CHROMAKEY(unsigned char, 0xff, 3, 0);
			break;
		case BC_YUV888:
			CHROMAKEY(unsigned char, 0xff, 3, 1);
			break;
		case BC_RGBA8888:
			CHROMAKEY(unsigned char, 0xff, 4, 0);
			break;
		case BC_YUVA8888:
			CHROMAKEY(unsigned char, 0xff, 4, 1);
			break;
		case BC_RGB161616:
			CHROMAKEY(uint16_t, 0xffff, 3, 0);
			break;
		case BC_YUV161616:
			CHROMAKEY(uint16_t, 0xffff, 3, 1);
			break;
		case BC_RGBA16161616:
			CHROMAKEY(uint16_t, 0xffff, 4, 0);
			break;
		case BC_YUVA16161616:
			CHROMAKEY(uint16_t, 0xffff, 4, 1);
			break;
	}

}





REGISTER_PLUGIN(ChromaKey)



ChromaKey::ChromaKey(PluginServer *server)
 : PluginVClient(server)
{
	PLUGIN_CONSTRUCTOR_MACRO
	engine = 0;
}

ChromaKey::~ChromaKey()
{
	PLUGIN_DESTRUCTOR_MACRO
	if(engine) delete engine;
}


int ChromaKey::process_realtime(VFrame *input, VFrame *output)
{
	load_configuration();
	this->input = input;
	this->output = output;

	if(EQUIV(config.threshold, 0))
	{
		if(input->get_rows()[0] != output->get_rows()[0])
			output->copy_from(input);
	}
	else
	{
		if(!engine) engine = new ChromaKeyServer(this);
		engine->process_packages();
	}

	return 0;
}

int ChromaKey::is_realtime()
{
	return 1;
}
char* ChromaKey::plugin_title()
{
	return "Chroma key";
}

NEW_PICON_MACRO(ChromaKey)

LOAD_CONFIGURATION_MACRO(ChromaKey, ChromaKeyConfig)

int ChromaKey::load_defaults()
{
	char directory[BCTEXTLEN];
// set the default directory
	sprintf(directory, "%schromakey.rc", BCASTDIR);

// load the defaults
	defaults = new Defaults(directory);
	defaults->load();

	config.hue = defaults->get("HUE", config.hue);
	config.value = defaults->get("VALUE", config.value);
	config.threshold = defaults->get("THRESHOLD", config.threshold);
	config.threshold = defaults->get("SLOPE", config.slope);
	config.use_value = defaults->get("USE_VALUE", config.use_value);
	return 0;
}

int ChromaKey::save_defaults()
{
	defaults->update("HUE", config.hue);
	defaults->update("VALUE", config.value);
    defaults->update("THRESHOLD", config.threshold);
    defaults->update("SLOPE", config.slope);
    defaults->update("USE_VALUE", config.use_value);
	defaults->save();
	return 0;
}

void ChromaKey::save_data(KeyFrame *keyframe)
{
	FileXML output;
	output.set_shared_string(keyframe->data, MESSAGESIZE);
	output.tag.set_title("CHROMAKEY");
	output.tag.set_property("HUE", config.hue);
	output.tag.set_property("VALUE", config.value);
	output.tag.set_property("THRESHOLD", config.threshold);
	output.tag.set_property("SLOPE", config.slope);
	output.tag.set_property("USE_VALUE", config.use_value);
	output.append_tag();
	output.terminate_string();
}

void ChromaKey::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->data, strlen(keyframe->data));

	while(!input.read_tag())
	{
		if(input.tag.title_is("CHROMAKEY"))
		{
			config.hue = input.tag.get_property("HUE", config.hue);
			config.value = input.tag.get_property("VALUE", config.value);
			config.threshold = input.tag.get_property("THRESHOLD", config.threshold);
			config.slope = input.tag.get_property("SLOPE", config.slope);
			config.use_value = input.tag.get_property("USE_VALUE", config.use_value);
		}
	}
}


SHOW_GUI_MACRO(ChromaKey, ChromaKeyThread)

SET_STRING_MACRO(ChromaKey)

RAISE_WINDOW_MACRO(ChromaKey)

void ChromaKey::update_gui()
{
	if(thread)
	{
		load_configuration();
		thread->window->lock_window();
		thread->window->hue->update(config.hue);
		thread->window->value->update(config.value);
		thread->window->threshold->update(config.threshold);
		thread->window->slope->update(config.slope);
		thread->window->use_value->update(config.use_value);
		thread->window->unlock_window();
	}
}








