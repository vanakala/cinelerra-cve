
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */

#include "bcdisplayinfo.h"
#include "clip.h"
#include "bchash.h"
#include "filexml.h"
#include "guicast.h"
#include "keyframe.h"
#include "language.h"
#include "loadbalance.h"
#include "picon_png.h"
#include "pluginvclient.h"
#include "vframe.h"

#include <math.h>
#include <stdint.h>
#include <string.h>




// Algorithm by Torsten Martinsen
// Ported to Cinelerra by Heroine Virtual Ltd.




class OilEffect;



class OilConfig
{
public:
	OilConfig();
	void copy_from(OilConfig &src);
	int equivalent(OilConfig &src);
	void interpolate(OilConfig &prev, 
		OilConfig &next, 
		long prev_frame, 
		long next_frame, 
		long current_frame);
	float radius;
	int use_intensity;
};

class OilRadius : public BC_FSlider
{
public:
	OilRadius(OilEffect *plugin, int x, int y);
	int handle_event();
	OilEffect *plugin;
};


class OilIntensity : public BC_CheckBox
{
public:
	OilIntensity(OilEffect *plugin, int x, int y);
	int handle_event();
	OilEffect *plugin;
};

class OilWindow : public BC_Window
{
public:
	OilWindow(OilEffect *plugin, int x, int y);
	~OilWindow();
	void create_objects();
	int close_event();
	OilEffect *plugin;
	OilRadius *radius;
	OilIntensity *intensity;
};

PLUGIN_THREAD_HEADER(OilEffect, OilThread, OilWindow)





class OilServer : public LoadServer
{
public:
	OilServer(OilEffect *plugin, int cpus);
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
	OilEffect *plugin;
};

class OilPackage : public LoadPackage
{
public:
	OilPackage();
	int row1, row2;
};

class OilUnit : public LoadClient
{
public:
	OilUnit(OilEffect *plugin, OilServer *server);
	void process_package(LoadPackage *package);
	OilEffect *plugin;
};









class OilEffect : public PluginVClient
{
public:
	OilEffect(PluginServer *server);
	~OilEffect();

	int process_realtime(VFrame *input, VFrame *output);
	int is_realtime();
	char* plugin_title();
	VFrame* new_picon();
	int load_configuration();
	int load_defaults();
	int save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	int show_gui();
	int set_string();
	void raise_window();
	void update_gui();

	OilConfig config;
	VFrame *temp_frame;
	VFrame *input, *output;
	BC_Hash *defaults;
	OilThread *thread;
	OilServer *engine;
	int need_reconfigure;
};













OilConfig::OilConfig()
{
	radius = 5;
	use_intensity = 0;
}

void OilConfig::copy_from(OilConfig &src)
{
	this->radius = src.radius;
	this->use_intensity = src.use_intensity;
}

int OilConfig::equivalent(OilConfig &src)
{
	return (EQUIV(this->radius, src.radius) &&
		this->use_intensity == src.use_intensity);
}

void OilConfig::interpolate(OilConfig &prev, 
		OilConfig &next, 
		long prev_frame, 
		long next_frame, 
		long current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);
	this->radius = prev.radius * prev_scale + next.radius * next_scale;
	this->use_intensity = prev.use_intensity;
// printf("OilConfig::interpolate prev_frame=%ld current_frame=%ld next_frame=%ld prev.radius=%f this->radius=%f next.radius=%f\n", 
// 	prev_frame, current_frame, next_frame, prev.radius, this->radius, next.radius);
}












OilRadius::OilRadius(OilEffect *plugin, int x, int y)
 : BC_FSlider(x, 
	   	y, 
		0,
		200,
		200, 
		(float)0, 
		(float)30,
		plugin->config.radius)
{
	this->plugin = plugin;
}
int OilRadius::handle_event()
{
	plugin->config.radius = get_value();
	plugin->send_configure_change();
	return 1;
}







OilIntensity::OilIntensity(OilEffect *plugin, int x, int y)
 : BC_CheckBox(x, y, plugin->config.use_intensity, _("Use intensity"))
{
	this->plugin = plugin;
}
int OilIntensity::handle_event()
{
	plugin->config.use_intensity = get_value();
	plugin->send_configure_change();
	return 1;
}







OilWindow::OilWindow(OilEffect *plugin, int x, int y)
 : BC_Window(plugin->gui_string, 
 	x, 
	y, 
	300, 
	160, 
	300, 
	160, 
	0, 
	0,
	1)
{
	this->plugin = plugin;
}

OilWindow::~OilWindow()
{
}

void OilWindow::create_objects()
{
	int x = 10, y = 10;
	add_subwindow(new BC_Title(x, y, _("Radius:")));
	add_subwindow(radius = new OilRadius(plugin, x + 70, y));
	y += 40;
	add_subwindow(intensity = new OilIntensity(plugin, x, y));
	
	show_window();
	flush();
}

WINDOW_CLOSE_EVENT(OilWindow)



PLUGIN_THREAD_OBJECT(OilEffect, OilThread, OilWindow)




REGISTER_PLUGIN(OilEffect)




OilEffect::OilEffect(PluginServer *server)
 : PluginVClient(server)
{
	temp_frame = 0;
	need_reconfigure = 1;
	engine = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

OilEffect::~OilEffect()
{
	PLUGIN_DESTRUCTOR_MACRO

	if(temp_frame) delete temp_frame;
	if(engine) delete engine;
}


char* OilEffect::plugin_title() { return N_("Oil painting"); }
int OilEffect::is_realtime() { return 1; }


NEW_PICON_MACRO(OilEffect)

SHOW_GUI_MACRO(OilEffect, OilThread)

RAISE_WINDOW_MACRO(OilEffect)

SET_STRING_MACRO(OilEffect)

void OilEffect::update_gui()
{
	if(thread)
	{
		thread->window->lock_window();
		load_configuration();
//printf("OilEffect::update_gui 1 %ld %f\n", get_source_position(), config.radius);

		thread->window->radius->update(config.radius);
		thread->window->intensity->update(config.use_intensity);
		thread->window->unlock_window();
	}
}


LOAD_CONFIGURATION_MACRO(OilEffect, OilConfig)

int OilEffect::load_defaults()
{
	char directory[BCTEXTLEN];
// set the default directory
	sprintf(directory, "%soilpainting.rc", BCASTDIR);

// load the defaults
	defaults = new BC_Hash(directory);
	defaults->load();

	config.radius = defaults->get("RADIUS", config.radius);
	config.use_intensity = defaults->get("USE_INTENSITY", config.use_intensity);
	return 0;
}

int OilEffect::save_defaults()
{
	defaults->update("RADIUS", config.radius);
	defaults->update("USE_INTENSITY", config.use_intensity);
	defaults->save();
	return 0;
}

void OilEffect::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_string(keyframe->data, MESSAGESIZE);
	output.tag.set_title("OIL_PAINTING");
	output.tag.set_property("RADIUS", config.radius);
	output.tag.set_property("USE_INTENSITY", config.use_intensity);
	output.append_tag();
	output.tag.set_title("/OIL_PAINTING");
	output.append_tag();
	output.terminate_string();
}

void OilEffect::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->data, strlen(keyframe->data));

	int result = 0;

	while(!input.read_tag())
	{
		if(input.tag.title_is("OIL_PAINTING"))
		{
			config.radius = input.tag.get_property("RADIUS", config.radius);
			config.use_intensity = input.tag.get_property("USE_INTENSITY", config.use_intensity);
		}
	}
}


int OilEffect::process_realtime(VFrame *input, VFrame *output)
{
	need_reconfigure |= load_configuration();



//printf("OilEffect::process_realtime %f %d\n", config.radius, config.use_intensity);
	this->input = input;
	this->output = output;

	if(EQUIV(config.radius, 0))
	{
		if(input->get_rows()[0] != output->get_rows()[0])
			output->copy_from(input);
	}
	else
	{
		if(input->get_rows()[0] == output->get_rows()[0])
		{
			if(!temp_frame) temp_frame = new VFrame(0,
				input->get_w(),
				input->get_h(),
				input->get_color_model());
			temp_frame->copy_from(input);
			this->input = temp_frame;
		}
		
		
		if(!engine)
		{
			engine = new OilServer(this, (PluginClient::smp + 1));
		}
		
		engine->process_packages();
	}
	
	
	
	return 0;
}






OilPackage::OilPackage()
 : LoadPackage()
{
}






OilUnit::OilUnit(OilEffect *plugin, OilServer *server)
 : LoadClient(server)
{
	this->plugin = plugin;
}


#define INTENSITY(p) ((unsigned int)(((p)[0]) * 77+ \
									((p)[1] * 150) + \
									((p)[2] * 29)) >> 8)


#define OIL_MACRO(type, hist_size, components) \
{ \
	type *src, *dest; \
	type val[components]; \
	int count[components], count2; \
	int *hist[components]; \
	int *hist2; \
	type **src_rows = (type**)plugin->input->get_rows(); \
 \
	for(int i = 0; i < components; i++) \
		hist[i] = new int[hist_size + 1]; \
	hist2 = new int[hist_size + 1]; \
 \
	for(int y1 = pkg->row1; y1 < pkg->row2; y1++) \
	{ \
		dest = (type*)plugin->output->get_rows()[y1]; \
 \
		if(!plugin->config.use_intensity) \
		{ \
			for(int x1 = 0; x1 < w; x1++) \
			{ \
				bzero(count, sizeof(count)); \
				bzero(val, sizeof(val)); \
				bzero(hist[0], sizeof(int) * (hist_size + 1)); \
				bzero(hist[1], sizeof(int) * (hist_size + 1)); \
				bzero(hist[2], sizeof(int) * (hist_size + 1)); \
				if (components == 4) bzero(hist[3], sizeof(int) * (hist_size + 1)); \
 \
				int x3 = CLIP((x1 - n), 0, w - 1); \
				int y3 = CLIP((y1 - n), 0, h - 1); \
				int x4 = CLIP((x1 + n + 1), 0, w - 1); \
				int y4 = CLIP((y1 + n + 1), 0, h - 1); \
 \
				for(int y2 = y3; y2 < y4; y2++) \
				{ \
					src = src_rows[y2]; \
					for(int x2 = x3; x2 < x4; x2++) \
					{ \
						int c; \
						int subscript; \
						type value; \
 \
                        value = src[x2 * components + 0]; \
						if(sizeof(type) == 4) \
						{ \
							subscript = (int)(value * hist_size); \
							CLAMP(subscript, 0, hist_size); \
						} \
						else \
							subscript = (int)value; \
 \
						if((c = ++hist[0][subscript]) > count[0]) \
						{ \
							val[0] = value; \
							count[0] = c; \
						} \
 \
                        value = src[x2 * components + 1]; \
						if(sizeof(type) == 4) \
						{ \
							subscript = (int)(value * hist_size); \
							CLAMP(subscript, 0, hist_size); \
						} \
						else \
							subscript = (int)value; \
 \
						if((c = ++hist[1][subscript]) > count[1]) \
						{ \
							val[1] = value; \
							count[1] = c; \
						} \
 \
                        value = src[x2 * components + 2]; \
						if(sizeof(type) == 4) \
						{ \
							subscript = (int)(value * hist_size); \
							CLAMP(subscript, 0, hist_size); \
						} \
						else \
							subscript = (int)value; \
 \
						if((c = ++hist[2][subscript]) > count[2]) \
						{ \
							val[2] = value; \
							count[2] = c; \
						} \
 \
						if(components == 4) \
						{ \
                        	value = src[x2 * components + 3]; \
							if(sizeof(type) == 4) \
							{ \
								subscript = (int)(value * hist_size); \
								CLAMP(subscript, 0, hist_size); \
							} \
							else \
								subscript = (int)value; \
 \
							if((c = ++hist[3][subscript]) > count[3]) \
							{ \
								val[3] = value; \
								count[3] = c; \
							} \
						} \
					} \
				} \
 \
				dest[x1 * components + 0] = val[0]; \
				dest[x1 * components + 1] = val[1]; \
				dest[x1 * components + 2] = val[2]; \
				if(components == 4) dest[x1 * components + 3] = val[3]; \
			} \
		} \
		else \
		{ \
			for(int x1 = 0; x1 < w; x1++) \
			{ \
				count2 = 0; \
				bzero(val, sizeof(val)); \
				bzero(hist2, sizeof(int) * (hist_size + 1)); \
 \
				int x3 = CLIP((x1 - n), 0, w - 1); \
	    		int y3 = CLIP((y1 - n), 0, h - 1); \
	    		int x4 = CLIP((x1 + n + 1), 0, w - 1); \
	    		int y4 = CLIP((y1 + n + 1), 0, h - 1); \
 \
				for(int y2 = y3; y2 < y4; y2++) \
				{ \
					src = src_rows[y2]; \
					for(int x2 = x3; x2 < x4; x2++) \
					{ \
						int c; \
						if((c = ++hist2[INTENSITY(src + x2 * components)]) > count2) \
						{ \
							val[0] = src[x2 * components + 0]; \
							val[1] = src[x2 * components + 1]; \
							val[2] = src[x2 * components + 2]; \
							if(components == 4) val[3] = src[x2 * components + 3]; \
							count2 = c; \
						} \
					} \
				} \
 \
				dest[x1 * components + 0] = val[0]; \
				dest[x1 * components + 1] = val[1]; \
				dest[x1 * components + 2] = val[2]; \
				if(components == 4) dest[x1 * components + 3] = val[3]; \
			} \
		} \
	} \
 \
	for(int i = 0; i < components; i++) \
		delete [] hist[i]; \
	delete [] hist2; \
}




void OilUnit::process_package(LoadPackage *package)
{
	OilPackage *pkg = (OilPackage*)package;
	int w = plugin->input->get_w();
	int h = plugin->input->get_h();
	int n = (int)(plugin->config.radius / 2);

	switch(plugin->input->get_color_model())
	{
		case BC_RGB_FLOAT:
			OIL_MACRO(float, 0xffff, 3)
			break;
		case BC_RGB888:
		case BC_YUV888:
			OIL_MACRO(unsigned char, 0xff, 3)
			break;
		case BC_RGB161616:
		case BC_YUV161616:
			OIL_MACRO(uint16_t, 0xffff, 3)
			break;
		case BC_RGBA_FLOAT:
			OIL_MACRO(float, 0xffff, 4)
			break;
		case BC_RGBA8888:
		case BC_YUVA8888:
			OIL_MACRO(unsigned char, 0xff, 4)
			break;
		case BC_RGBA16161616:
		case BC_YUVA16161616:
			OIL_MACRO(uint16_t, 0xffff, 4)
			break;
	}




}






OilServer::OilServer(OilEffect *plugin, int cpus)
 : LoadServer(cpus, cpus)
{
	this->plugin = plugin;
}

void OilServer::init_packages()
{
	for(int i = 0; i < LoadServer::get_total_packages(); i++)
	{
		OilPackage *pkg = (OilPackage*)get_package(i);
		pkg->row1 = plugin->input->get_h() * i / LoadServer::get_total_packages();
		pkg->row2 = plugin->input->get_h() * (i + 1) / LoadServer::get_total_packages();
	}
}


LoadClient* OilServer::new_client()
{
	return new OilUnit(plugin, this);
}

LoadPackage* OilServer::new_package()
{
	return new OilPackage;
}

