#include "bcdisplayinfo.h"
#include "clip.h"
#include "defaults.h"
#include "filexml.h"
#include "guicast.h"
#include "picon_png.h"
#include "../colors/plugincolors.h"
#include "pluginvclient.h"
#include "vframe.h"

#include <stdint.h>
#include <string.h>


class InvertVideoEffect;


class InvertVideoConfig
{
public:
	InvertVideoConfig();

	void copy_from(InvertVideoConfig &src);
	int equivalent(InvertVideoConfig &src);
	void interpolate(InvertVideoConfig &prev, 
		InvertVideoConfig &next, 
		long prev_frame, 
		long next_frame, 
		long current_frame);

	int r, g, b, a;
};

class InvertVideoEnable : public BC_CheckBox
{
public:
	InvertVideoEnable(InvertVideoEffect *plugin, int *output, int x, int y, char *text);
	int handle_event();
	InvertVideoEffect *plugin;
	int *output;
};

class InvertVideoWindow : public BC_Window
{
public:
	InvertVideoWindow(InvertVideoEffect *plugin, int x, int y);
	void create_objects();
	int close_event();
	InvertVideoEnable *r, *g, *b, *a;
	InvertVideoEffect *plugin;
};

PLUGIN_THREAD_HEADER(InvertVideoEffect, InvertVideoThread, InvertVideoWindow)

class InvertVideoEffect : public PluginVClient
{
public:
	InvertVideoEffect(PluginServer *server);
	~InvertVideoEffect();
	int process_realtime(VFrame *input, VFrame *output);
	int is_realtime();
	char* plugin_title();
	VFrame* new_picon();
	int load_defaults();
	int save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void update_gui();
	int show_gui();
	void raise_window();
	int set_string();
	int load_configuration();

	InvertVideoConfig config;
	InvertVideoThread *thread;
	Defaults *defaults;
};





REGISTER_PLUGIN(InvertVideoEffect)







InvertVideoConfig::InvertVideoConfig()
{
	r = 1;
	g = 1;
	b = 1;
	a = 1;
}

void InvertVideoConfig::copy_from(InvertVideoConfig &src)
{
	r = src.r;
	g = src.g;
	b = src.b;
	a = src.a;
}

int InvertVideoConfig::equivalent(InvertVideoConfig &src)
{
	return r == src.r && 
		g == src.g && 
		b == src.b && 
		a == src.a;
}

void InvertVideoConfig::interpolate(InvertVideoConfig &prev, 
	InvertVideoConfig &next, 
	long prev_frame, 
	long next_frame, 
	long current_frame)
{
	r = prev.r;
	g = prev.g;
	b = prev.b;
	a = prev.a;
}




InvertVideoEnable::InvertVideoEnable(InvertVideoEffect *plugin, int *output, int x, int y, char *text)
 : BC_CheckBox(x, y, *output, text)
{
	this->plugin = plugin;
	this->output = output;
}
int InvertVideoEnable::handle_event()
{
	*output = get_value();
	plugin->send_configure_change();
	return 1;
}





InvertVideoWindow::InvertVideoWindow(InvertVideoEffect *plugin, int x, int y)
 : BC_Window(plugin->gui_string, 
 	x, 
	y, 
	260, 
	130, 
	260, 
	130, 
	0, 
	0,
	1)
{
	this->plugin = plugin;
}

void InvertVideoWindow::create_objects()
{
	int x = 10, y = 10;
	add_subwindow(r = new InvertVideoEnable(plugin, &plugin->config.r, x, y, "Invert R"));
	y += 30;
	add_subwindow(g = new InvertVideoEnable(plugin, &plugin->config.g, x, y, "Invert G"));
	y += 30;
	add_subwindow(b = new InvertVideoEnable(plugin, &plugin->config.b, x, y, "Invert B"));
	y += 30;
	add_subwindow(a = new InvertVideoEnable(plugin, &plugin->config.a, x, y, "Invert A"));

	show_window();
	flush();
}

int InvertVideoWindow::close_event()
{
// Set result to 1 to indicate a client side close
	set_done(1);
	return 1;
}





PLUGIN_THREAD_OBJECT(InvertVideoEffect, InvertVideoThread, InvertVideoWindow)






InvertVideoEffect::InvertVideoEffect(PluginServer *server)
 : PluginVClient(server)
{
	PLUGIN_CONSTRUCTOR_MACRO
}
InvertVideoEffect::~InvertVideoEffect()
{
	PLUGIN_DESTRUCTOR_MACRO
}
int InvertVideoEffect::is_realtime()
{
	return 1;
}

char* InvertVideoEffect::plugin_title()
{
	return "Invert Video";
}

NEW_PICON_MACRO(InvertVideoEffect)
SHOW_GUI_MACRO(InvertVideoEffect, InvertVideoThread)
RAISE_WINDOW_MACRO(InvertVideoEffect)
SET_STRING_MACRO(InvertVideoEffect)
LOAD_CONFIGURATION_MACRO(InvertVideoEffect, InvertVideoConfig)

void InvertVideoEffect::update_gui()
{
	if(thread)
	{
		thread->window->lock_window();
		load_configuration();
		thread->window->r->update(config.r);
		thread->window->g->update(config.g);
		thread->window->b->update(config.b);
		thread->window->a->update(config.a);
		thread->window->unlock_window();
	}
}

int InvertVideoEffect::load_defaults()
{
	char directory[BCTEXTLEN];
	sprintf(directory, "%sinvertvideo.rc", BCASTDIR);
	defaults = new Defaults(directory);
	defaults->load();
	config.r = defaults->get("R", config.r);
	config.g = defaults->get("G", config.g);
	config.b = defaults->get("B", config.b);
	config.a = defaults->get("A", config.a);
	return 0;
}

int InvertVideoEffect::save_defaults()
{
	defaults->update("R", config.r);
	defaults->update("G", config.g);
	defaults->update("B", config.b);
	defaults->update("A", config.a);
	defaults->save();
	return 0;
}

void InvertVideoEffect::save_data(KeyFrame *keyframe)
{
	FileXML output;
	output.set_shared_string(keyframe->data, MESSAGESIZE);
	output.tag.set_title("INVERTVIDEO");
	output.tag.set_property("R", config.r);
	output.tag.set_property("G", config.g);
	output.tag.set_property("B", config.b);
	output.tag.set_property("A", config.a);
	output.append_tag();
	output.terminate_string();
}

void InvertVideoEffect::read_data(KeyFrame *keyframe)
{
	FileXML input;
	input.set_shared_string(keyframe->data, strlen(keyframe->data));
	while(!input.read_tag())
	{
		if(input.tag.title_is("INVERTVIDEO"))
		{
			config.r = input.tag.get_property("R", config.r);
			config.g = input.tag.get_property("G", config.g);
			config.b = input.tag.get_property("B", config.b);
			config.a = input.tag.get_property("A", config.a);
		}
	}
}


#define INVERT_MACRO(type, components, max) \
{ \
	for(int i = 0; i < input->get_h(); i++) \
	{ \
		type *in_row = (type*)input->get_rows()[i]; \
		type *out_row = (type*)output->get_rows()[i]; \
 \
		for(int j = 0; j < w; j++) \
		{ \
			if(config.r) out_row[0] = max - in_row[0]; \
			if(config.g) out_row[1] = max - in_row[1]; \
			if(config.b) out_row[2] = max - in_row[2]; \
			if(components == 4) \
				if(config.a) out_row[3] = max - in_row[3]; \
 \
			in_row += components; \
			out_row += components; \
		} \
	} \
}

int InvertVideoEffect::process_realtime(VFrame *input, VFrame *output)
{
	load_configuration();

	if(!config.r && !config.g && !config.b && !config.a)
	{
		if(input->get_rows()[0] != output->get_rows()[0])
			output->copy_from(input);
	}
	else
	{
		int w = input->get_w();

		switch(input->get_color_model())
		{
			case BC_RGB888:
			case BC_YUV888:
				INVERT_MACRO(unsigned char, 3, 0xff)
				break;
			case BC_RGBA8888:
			case BC_YUVA8888:
				INVERT_MACRO(unsigned char, 4, 0xff)
				break;
			case BC_RGB161616:
			case BC_YUV161616:
				INVERT_MACRO(uint16_t, 3, 0xffff)
				break;
			case BC_RGBA16161616:
			case BC_YUVA16161616:
				INVERT_MACRO(uint16_t, 4, 0xffff)
				break;
		}
	}

	return 0;
}


