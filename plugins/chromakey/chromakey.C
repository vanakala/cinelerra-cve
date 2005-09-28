#include "bcdisplayinfo.h"
#include "chromakey.h"
#include "clip.h"
#include "defaults.h"
#include "filexml.h"
#include "guicast.h"
#include "keyframe.h"
#include "language.h"
#include "loadbalance.h"
#include "picon_png.h"
#include "plugincolors.h"
#include "pluginvclient.h"
#include "vframe.h"

#include <stdint.h>
#include <string.h>





ChromaKeyConfig::ChromaKeyConfig()
{
	red = 0.0;
	green = 0.0;
	blue = 0.0;
	threshold = 60.0;
	use_value = 0;
	slope = 100;
}


void ChromaKeyConfig::copy_from(ChromaKeyConfig &src)
{
	red = src.red;
	green = src.green;
	blue = src.blue;
	threshold = src.threshold;
	use_value = src.use_value;
	slope = src.slope;
}

int ChromaKeyConfig::equivalent(ChromaKeyConfig &src)
{
	return (EQUIV(red, src.red) &&
		EQUIV(green, src.green) &&
		EQUIV(blue, src.blue) &&
		EQUIV(threshold, src.threshold) &&
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

	this->red = prev.red * prev_scale + next.red * next_scale;
	this->green = prev.green * prev_scale + next.green * next_scale;
	this->blue = prev.blue * prev_scale + next.blue * next_scale;
	this->threshold = prev.threshold * prev_scale + next.threshold * next_scale;
	this->slope = prev.slope * prev_scale + next.slope * next_scale;
	this->use_value = prev.use_value;
}

int ChromaKeyConfig::get_color()
{
	int red = (int)(CLIP(this->red, 0, 1) * 0xff);
	int green = (int)(CLIP(this->green, 0, 1) * 0xff);
	int blue = (int)(CLIP(this->blue, 0, 1) * 0xff);
	return (red << 16) | (green << 8) | blue;
}







ChromaKeyWindow::ChromaKeyWindow(ChromaKey *plugin, int x, int y)
 : BC_Window(plugin->gui_string, 
 	x, 
	y, 
	320, 
	220, 
	320, 
	220, 
	0, 
	0,
	1)
{
	this->plugin = plugin;
	color_thread = 0;
}

ChromaKeyWindow::~ChromaKeyWindow()
{
	delete color_thread;
}

void ChromaKeyWindow::create_objects()
{
	int x = 10, y = 10, x1 = 100;

	BC_Title *title;
	add_subwindow(title = new BC_Title(x, y, _("Color:")));
	x += title->get_w() + 10;
	add_subwindow(color = new ChromaKeyColor(plugin, this, x, y));
	x += color->get_w() + 10;
	add_subwindow(sample = new BC_SubWindow(x, y, 100, 50));
	y += sample->get_h() + 10;
	x = 10;

	add_subwindow(new BC_Title(x, y, _("Slope:")));
	add_subwindow(slope = new ChromaKeySlope(plugin, x1, y));

	y += 30;
	add_subwindow(new BC_Title(x, y, _("Threshold:")));
	add_subwindow(threshold = new ChromaKeyThreshold(plugin, x1, y));


	y += 30;
	add_subwindow(use_value = new ChromaKeyUseValue(plugin, x1, y));

	y += 30;
	add_subwindow(use_colorpicker = new ChromaKeyUseColorPicker(plugin, this, x1, y));

	color_thread = new ChromaKeyColorThread(plugin, this);

	update_sample();
	show_window();
	flush();
}

void ChromaKeyWindow::update_sample()
{
	sample->set_color(plugin->config.get_color());
	sample->draw_box(0, 
		0, 
		sample->get_w(), 
		sample->get_h());
	sample->set_color(BLACK);
	sample->draw_rectangle(0, 
		0, 
		sample->get_w(), 
		sample->get_h());
	sample->flash();
}



WINDOW_CLOSE_EVENT(ChromaKeyWindow)






ChromaKeyColor::ChromaKeyColor(ChromaKey *plugin, 
	ChromaKeyWindow *gui, 
	int x, 
	int y)
 : BC_GenericButton(x, 
	y,
	_("Color..."))
{
	this->plugin = plugin;
	this->gui = gui;
}
int ChromaKeyColor::handle_event()
{
	gui->color_thread->start_window(
		plugin->config.get_color(),
		0xff);
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
 : BC_CheckBox(x, y, plugin->config.use_value, _("Use value"))
{
	this->plugin = plugin;
}
int ChromaKeyUseValue::handle_event()
{
	plugin->config.use_value = get_value();
	plugin->send_configure_change();
	return 1;
}


ChromaKeyUseColorPicker::ChromaKeyUseColorPicker(ChromaKey *plugin, 
	ChromaKeyWindow *gui,
	int x, 
	int y)
 : BC_GenericButton(x, y, _("Use color picker"))
{
	this->plugin = plugin;
	this->gui = gui;
}

int ChromaKeyUseColorPicker::handle_event()
{
	plugin->config.red = plugin->get_red();
	plugin->config.green = plugin->get_green();
	plugin->config.blue = plugin->get_blue();
	gui->update_sample();
	plugin->send_configure_change();
	return 1;
}




ChromaKeyColorThread::ChromaKeyColorThread(ChromaKey *plugin, ChromaKeyWindow *gui)
 : ColorThread(1, _("Inner color"))
{
	this->plugin = plugin;
	this->gui = gui;
}

int ChromaKeyColorThread::handle_new_color(int output, int alpha)
{
	plugin->config.red = (float)(output & 0xff0000) / 0xff0000;
	plugin->config.green = (float)(output & 0xff00) / 0xff00;
	plugin->config.blue = (float)(output & 0xff) / 0xff;
	gui->update_sample();
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
	for(int i = 0; i < get_total_packages(); i++)
	{
		ChromaKeyPackage *pkg = (ChromaKeyPackage*)get_package(i);
		pkg->y1 = plugin->input->get_h() * i / get_total_packages();
		pkg->y2 = plugin->input->get_h() * (i + 1) / get_total_packages();
	}
	
}
LoadClient* ChromaKeyServer::new_client()
{
	return new ChromaKeyUnit(plugin, this);
}
LoadPackage* ChromaKeyServer::new_package()
{
	return new ChromaKeyPackage;
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

	float h, s, v;
	HSV::rgb_to_hsv(plugin->config.red, 
		plugin->config.green,
		plugin->config.blue,
		h,
		s,
		v);
 	float min_hue = h - plugin->config.threshold * 360 / 100;
 	float max_hue = h + plugin->config.threshold * 360 / 100;


#define RGB_TO_VALUE(r, g, b) \
((r) * R_TO_Y + (g) * G_TO_Y + (b) * B_TO_Y)

#define SQR(x) ((x) * (x))

	float value = RGB_TO_VALUE(plugin->config.red,
		plugin->config.green,
		plugin->config.blue);
	float min_v = value - plugin->config.threshold / 100;
	float max_v = value + plugin->config.threshold / 100;
	float threshold = plugin->config.threshold / 100;
	float red = plugin->config.red;
	float green = plugin->config.green;
	float blue = plugin->config.blue;

	float run = plugin->config.slope / 100;
	float threshold_run = threshold + run;


#define CHROMAKEY(type, components, max, use_yuv) \
{ \
	for(int i = pkg->y1; i < pkg->y2; i++) \
	{ \
		type *row = (type*)plugin->input->get_rows()[i]; \
 \
		for(int j = 0; j < w; j++) \
		{ \
			float a = 1; \
 \
/* Test for value in range */ \
			if(plugin->config.use_value) \
			{ \
				float current_value; \
				if(use_yuv) \
				{ \
					float r = (float)row[0] / max; \
					current_value = r; \
				} \
				else \
				{ \
					float r = (float)row[0] / max; \
					float g = (float)row[1] / max; \
					float b = (float)row[2] / max; \
					current_value = RGB_TO_VALUE(r, g, b); \
				} \
 \
/* Full transparency if in range */ \
				if(current_value >= min_v && current_value < max_v) \
				{ \
					a = 0; \
				} \
				else \
/* Phased out if below or above range */ \
				if(current_value < min_v) \
				{ \
					if(min_v - current_value < run) \
						a = (min_v - current_value) / run; \
				} \
				else \
				if(current_value - max_v < run) \
					a = (current_value - max_v) / run; \
			} \
			else \
/* Use color cube */ \
			{ \
				float r = (float)row[0] / max; \
				float g = (float)row[1] / max; \
				float b = (float)row[2] / max; \
				if(use_yuv) \
				{ \
/* Convert pixel to RGB float */ \
					float y = r; \
					float u = g; \
					float v = b; \
					YUV::yuv_to_rgb_f(r, g, b, y, u - 0.5, v - 0.5); \
				} \
 \
				float difference = sqrt(SQR(r - red) +  \
					SQR(g - green) + \
					SQR(b - blue)); \
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
 \
			} \
 \
/* Multiply alpha and put back in frame */ \
			if(components == 4) \
			{ \
				row[3] = MIN((type)(a * max), row[3]); \
			} \
			else \
			if(use_yuv) \
			{ \
				row[0] = (type)(a * row[0]); \
				row[1] = (type)(a * (row[1] - (max / 2 + 1)) + max / 2 + 1); \
				row[2] = (type)(a * (row[2] - (max / 2 + 1)) + max / 2 + 1); \
			} \
			else \
			{ \
				row[0] = (type)(a * row[0]); \
				row[1] = (type)(a * row[1]); \
				row[2] = (type)(a * row[2]); \
			} \
 \
			row += components; \
		} \
	} \
}




	switch(plugin->input->get_color_model())
	{
		case BC_RGB_FLOAT:
			CHROMAKEY(float, 3, 1.0, 0);
			break;
		case BC_RGBA_FLOAT:
			CHROMAKEY(float, 4, 1.0, 0);
			break;
		case BC_RGB888:
			CHROMAKEY(unsigned char, 3, 0xff, 0);
			break;
		case BC_RGBA8888:
			CHROMAKEY(unsigned char, 4, 0xff, 0);
			break;
		case BC_YUV888:
			CHROMAKEY(unsigned char, 3, 0xff, 1);
			break;
		case BC_YUVA8888:
			CHROMAKEY(unsigned char, 4, 0xff, 1);
			break;
		case BC_YUV161616:
			CHROMAKEY(uint16_t, 3, 0xffff, 1);
			break;
		case BC_YUVA16161616:
			CHROMAKEY(uint16_t, 4, 0xffff, 1);
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

char* ChromaKey::plugin_title() { return N_("Chroma key"); }
int ChromaKey::is_realtime() { return 1; }

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

	config.red = defaults->get("RED", config.red);
	config.green = defaults->get("GREEN", config.green);
	config.blue = defaults->get("BLUE", config.blue);
	config.threshold = defaults->get("THRESHOLD", config.threshold);
	config.slope = defaults->get("SLOPE", config.slope);
	config.use_value = defaults->get("USE_VALUE", config.use_value);
	return 0;
}

int ChromaKey::save_defaults()
{
	defaults->update("RED", config.red);
	defaults->update("GREEN", config.green);
	defaults->update("BLUE", config.blue);
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
	output.tag.set_property("RED", config.red);
	output.tag.set_property("GREEN", config.green);
	output.tag.set_property("BLUE", config.blue);
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
			config.red = input.tag.get_property("RED", config.red);
			config.green = input.tag.get_property("GREEN", config.green);
			config.blue = input.tag.get_property("BLUE", config.blue);
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
		thread->window->threshold->update(config.threshold);
		thread->window->slope->update(config.slope);
		thread->window->use_value->update(config.use_value);
		thread->window->update_sample();

		thread->window->unlock_window();
	}
}








