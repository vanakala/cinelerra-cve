
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

#include <math.h>
#include <stdint.h>
#include <string.h>

#include "bcdisplayinfo.h"
#include "clip.h"
#include "bchash.h"
#include "filexml.h"
#include "keyframe.h"
#include "language.h"
#include "loadbalance.h"
#include "picon_png.h"
#include "pluginvclient.h"
#include "vframe.h"


class MotionBlurMain;
class MotionBlurEngine;





class MotionBlurConfig
{
public:
	MotionBlurConfig();

	int equivalent(MotionBlurConfig &that);
	void copy_from(MotionBlurConfig &that);
	void interpolate(MotionBlurConfig &prev, 
		MotionBlurConfig &next, 
		long prev_frame, 
		long next_frame, 
		long current_frame);

	int radius;
	int steps;
	int r;
	int g;
	int b;
	int a;
};



class MotionBlurSize : public BC_ISlider
{
public:
	MotionBlurSize(MotionBlurMain *plugin, 
		int x, 
		int y, 
		int *output,
		int min,
		int max);
	int handle_event();
	MotionBlurMain *plugin;
	int *output;
};


class MotionBlurWindow : public BC_Window
{
public:
	MotionBlurWindow(MotionBlurMain *plugin, int x, int y);
	~MotionBlurWindow();

	int create_objects();
	int close_event();

	MotionBlurSize *steps, *radius;
	MotionBlurMain *plugin;
};



PLUGIN_THREAD_HEADER(MotionBlurMain, MotionBlurThread, MotionBlurWindow)


class MotionBlurMain : public PluginVClient
{
public:
	MotionBlurMain(PluginServer *server);
	~MotionBlurMain();

	int process_realtime(VFrame *input_ptr, VFrame *output_ptr);
	int is_realtime();
	int load_defaults();
	int save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void update_gui();

	PLUGIN_CLASS_MEMBERS(MotionBlurConfig, MotionBlurThread)

	void delete_tables();
	VFrame *input, *output, *temp;
	MotionBlurEngine *engine;
	int **scale_y_table;
	int **scale_x_table;
	int table_entries;
	unsigned char *accum;
};

class MotionBlurPackage : public LoadPackage
{
public:
	MotionBlurPackage();
	int y1, y2;
};

class MotionBlurUnit : public LoadClient
{
public:
	MotionBlurUnit(MotionBlurEngine *server, MotionBlurMain *plugin);
	void process_package(LoadPackage *package);
	MotionBlurEngine *server;
	MotionBlurMain *plugin;
};

class MotionBlurEngine : public LoadServer
{
public:
	MotionBlurEngine(MotionBlurMain *plugin, 
		int total_clients, 
		int total_packages);
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
	MotionBlurMain *plugin;
};



















REGISTER_PLUGIN(MotionBlurMain)



MotionBlurConfig::MotionBlurConfig()
{
	radius = 10;
	steps = 10;
	r = 1;
	g = 1;
	b = 1;
	a = 1;
}

int MotionBlurConfig::equivalent(MotionBlurConfig &that)
{
	return 
		radius == that.radius &&
		steps == that.steps &&
		r == that.r &&
		g == that.g &&
		b == that.b &&
		a == that.a;
}

void MotionBlurConfig::copy_from(MotionBlurConfig &that)
{
	radius = that.radius;
	steps = that.steps;
	r = that.r;
	g = that.g;
	b = that.b;
	a = that.a;
}

void MotionBlurConfig::interpolate(MotionBlurConfig &prev, 
	MotionBlurConfig &next, 
	long prev_frame, 
	long next_frame, 
	long current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);
	this->radius = (int)(prev.radius * prev_scale + next.radius * next_scale + 0.5);
	this->steps = (int)(prev.steps * prev_scale + next.steps * next_scale + 0.5);
	r = prev.r;
	g = prev.g;
	b = prev.b;
	a = prev.a;
}









PLUGIN_THREAD_OBJECT(MotionBlurMain, MotionBlurThread, MotionBlurWindow)



MotionBlurWindow::MotionBlurWindow(MotionBlurMain *plugin, int x, int y)
 : BC_Window(plugin->gui_string, 
 	x,
	y,
	260, 
	120, 
	260, 
	120, 
	0, 
	1)
{
	this->plugin = plugin; 
}

MotionBlurWindow::~MotionBlurWindow()
{
}

int MotionBlurWindow::create_objects()
{
	int x = 10, y = 10;

	add_subwindow(new BC_Title(x, y, _("Length:")));
	y += 20;
	add_subwindow(radius = new MotionBlurSize(plugin, x, y, &plugin->config.radius, 0, 100));
	y += 30;
	add_subwindow(new BC_Title(x, y, _("Steps:")));
	y += 20;
	add_subwindow(steps = new MotionBlurSize(plugin, x, y, &plugin->config.steps, 1, 100));

	show_window();
	flush();
	return 0;
}

WINDOW_CLOSE_EVENT(MotionBlurWindow)



MotionBlurSize::MotionBlurSize(MotionBlurMain *plugin, 
	int x, 
	int y, 
	int *output,
	int min,
	int max)
 : BC_ISlider(x, y, 0, 240, 240, min, max, *output)
{
	this->plugin = plugin;
	this->output = output;
}
int MotionBlurSize::handle_event()
{
	*output = get_value();
	plugin->send_configure_change();
	return 1;
}










MotionBlurMain::MotionBlurMain(PluginServer *server)
 : PluginVClient(server)
{
	PLUGIN_CONSTRUCTOR_MACRO
	engine = 0;
	scale_x_table = 0;
	scale_y_table = 0;
	table_entries = 0;
	accum = 0;
	temp = 0;
}

MotionBlurMain::~MotionBlurMain()
{
	PLUGIN_DESTRUCTOR_MACRO
	if(engine) delete engine;
	delete_tables();
	if(accum) delete [] accum;
	if(temp) delete temp;
}

const char* MotionBlurMain::plugin_title() { return N_("Motion Blur"); }
int MotionBlurMain::is_realtime() { return 1; }


NEW_PICON_MACRO(MotionBlurMain)

SHOW_GUI_MACRO(MotionBlurMain, MotionBlurThread)

SET_STRING_MACRO(MotionBlurMain)

RAISE_WINDOW_MACRO(MotionBlurMain)

LOAD_CONFIGURATION_MACRO(MotionBlurMain, MotionBlurConfig)

void MotionBlurMain::delete_tables()
{
	if(scale_x_table)
	{
		for(int i = 0; i < table_entries; i++)
			delete [] scale_x_table[i];
		delete [] scale_x_table;
	}

	if(scale_y_table)
	{
		for(int i = 0; i < table_entries; i++)
			delete [] scale_y_table[i];
		delete [] scale_y_table;
	}
	scale_x_table = 0;
	scale_y_table = 0;
	table_entries = 0;
}

int MotionBlurMain::process_realtime(VFrame *input_ptr, VFrame *output_ptr)
{
	float xa,ya,za,xb,yb,zb,xd,yd,zd;
	if (get_source_position() == 0)	
		get_camera(&xa, &ya, &za, get_source_position());
	else
		get_camera(&xa, &ya, &za, get_source_position()-1);
	get_camera(&xb, &yb, &zb, get_source_position());

	xd = xb - xa;
	yd = yb - ya;
	zd = zb - za;
	
	//printf("Camera automation deltas: %.2f %.2f %.2f\n", xd, yd, zd);
	load_configuration();

//printf("MotionBlurMain::process_realtime 1 %d\n", config.radius);
	if(!engine) engine = new MotionBlurEngine(this,
		get_project_smp() + 1,
		get_project_smp() + 1);
	if(!accum) accum = new unsigned char[input_ptr->get_w() * 
		input_ptr->get_h() *
		cmodel_components(input_ptr->get_color_model()) *
		MAX(sizeof(int), sizeof(float))];

	this->input = input_ptr;
	this->output = output_ptr;


	if(input_ptr->get_rows()[0] == output_ptr->get_rows()[0])
	{
		if(!temp) temp = new VFrame(0,
			input_ptr->get_w(),
			input_ptr->get_h(),
			input_ptr->get_color_model());
		temp->copy_from(input_ptr);
		this->input = temp;
	}

// Generate tables here.  The same table is used by many packages to render
// each horizontal stripe.  Need to cover the entire output range in  each
// table to avoid green borders
	float w = input->get_w();
	float h = input->get_h();
	int x_offset;
	int y_offset;

	float fradius = config.radius * 0.5;
	float zradius = (float)(zd * fradius / 4 + 1);
	float center_x = w/2;
	float center_y = h/2;

	float min_w, min_h;
	float max_w, max_h;
	float min_x1, min_y1, min_x2, min_y2;
	float max_x1, max_y1, max_x2, max_y2;

	int steps = config.steps ? config.steps : 1;

	x_offset = (int)(xd * fradius);
	y_offset = (int)(yd * fradius);

    min_w = w * zradius;
    min_h = h * zradius;
    max_w = w;
    max_h = h;
    min_x1 = center_x - min_w / 2;
	min_y1 = center_y - min_h / 2;
	min_x2 = center_x + min_w / 2;
	min_y2 = center_y + min_h / 2;
	max_x1 = 0;
	max_y1 = 0;
	max_x2 = w;
	max_y2 = h;
		
	delete_tables();
	scale_x_table = new int*[config.steps];
	scale_y_table = new int*[config.steps];
	table_entries = config.steps;

	for(int i = 0; i < config.steps; i++)
	{
		float fraction = (float)(i - config.steps / 2) / config.steps;
		float inv_fraction = 1.0 - fraction;
		
		int x = (int)(fraction * x_offset);
		int y = (int)(fraction * y_offset);
		float out_x1 = min_x1 * fraction + max_x1 * inv_fraction;
		float out_x2 = min_x2 * fraction + max_x2 * inv_fraction;
		float out_y1 = min_y1 * fraction + max_y1 * inv_fraction;
		float out_y2 = min_y2 * fraction + max_y2 * inv_fraction;
		float out_w = out_x2 - out_x1;
		float out_h = out_y2 - out_y1;
		if(out_w < 0) out_w = 0;
		if(out_h < 0) out_h = 0;
		float scale_x = (float)w / out_w;
		float scale_y = (float)h / out_h;

		int *x_table;
		int *y_table;
                scale_y_table[i] = y_table = new int[(int)(h + 1)];
                scale_x_table[i] = x_table = new int[(int)(w + 1)];
			
		for(int j = 0; j < h; j++)
		{
			y_table[j] = (int)((j - out_y1) * scale_y) + y;
		}
		for(int j = 0; j < w; j++)
		{
			x_table[j] = (int)((j - out_x1) * scale_x) + x;
		}
	}

	bzero(accum, 
		input_ptr->get_w() * 
		input_ptr->get_h() * 
		cmodel_components(input_ptr->get_color_model()) * 
		MAX(sizeof(int), sizeof(float)));
	engine->process_packages();
	return 0;
}


void MotionBlurMain::update_gui()
{
	if(thread)
	{
		load_configuration();
		thread->window->lock_window();
		thread->window->radius->update(config.radius);
		thread->window->steps->update(config.steps);
		thread->window->unlock_window();
	}
}


int MotionBlurMain::load_defaults()
{
	char directory[1024], string[1024];
// set the default directory
	sprintf(directory, "%smotionblur.rc", BCASTDIR);

// load the defaults
	defaults = new BC_Hash(directory);
	defaults->load();

	config.radius = defaults->get("RADIUS", config.radius);
	config.steps = defaults->get("STEPS", config.steps);
	return 0;
}


int MotionBlurMain::save_defaults()
{
	defaults->update("RADIUS", config.radius);
	defaults->update("STEPS", config.steps);
	defaults->save();
	return 0;
}



void MotionBlurMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_string(keyframe->data, MESSAGESIZE);
	output.tag.set_title("MOTIONBLUR");

	output.tag.set_property("RADIUS", config.radius);
	output.tag.set_property("STEPS", config.steps);
	output.append_tag();
	output.tag.set_title("/MOTIONBLUR");
	output.append_tag();
	output.terminate_string();
}

void MotionBlurMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->data, strlen(keyframe->data));

	int result = 0;

	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("MOTIONBLUR"))
			{
				config.radius = input.tag.get_property("RADIUS", config.radius);
				config.steps = input.tag.get_property("STEPS", config.steps);
			}
		}
	}
}






MotionBlurPackage::MotionBlurPackage()
 : LoadPackage()
{
}




MotionBlurUnit::MotionBlurUnit(MotionBlurEngine *server, 
	MotionBlurMain *plugin)
 : LoadClient(server)
{
	this->plugin = plugin;
	this->server = server;
}


#define BLEND_LAYER(COMPONENTS, TYPE, TEMP, MAX, DO_YUV) \
{ \
	const int chroma_offset = (DO_YUV ? ((MAX + 1) / 2) : 0); \
	for(int j = pkg->y1; j < pkg->y2; j++) \
	{ \
		TEMP *out_row = (TEMP*)plugin->accum + COMPONENTS * w * j; \
		int in_y = y_table[j]; \
 \
/* Blend image */ \
		if(in_y >= 0 && in_y < h) \
		{ \
			TYPE *in_row = (TYPE*)plugin->input->get_rows()[in_y]; \
			for(int k = 0; k < w; k++) \
			{ \
				int in_x = x_table[k]; \
/* Blend pixel */ \
				if(in_x >= 0 && in_x < w) \
				{ \
					int in_offset = in_x * COMPONENTS; \
					*out_row++ += in_row[in_offset]; \
					if(DO_YUV) \
					{ \
						*out_row++ += in_row[in_offset + 1]; \
						*out_row++ += in_row[in_offset + 2]; \
					} \
					else \
					{ \
						*out_row++ += in_row[in_offset + 1]; \
						*out_row++ += in_row[in_offset + 2]; \
					} \
					if(COMPONENTS == 4) \
						*out_row++ += in_row[in_offset + 3]; \
				} \
/* Blend nothing */ \
				else \
				{ \
					out_row++; \
					if(DO_YUV) \
					{ \
						*out_row++ += chroma_offset; \
						*out_row++ += chroma_offset; \
					} \
					else \
					{ \
						out_row += 2; \
					} \
					if(COMPONENTS == 4) out_row++; \
				} \
			} \
		} \
		else \
		if(DO_YUV) \
		{ \
			for(int k = 0; k < w; k++) \
			{ \
				out_row++; \
				*out_row++ += chroma_offset; \
				*out_row++ += chroma_offset; \
				if(COMPONENTS == 4) out_row++; \
			} \
		} \
	} \
 \
/* Copy to output */ \
	if(i == plugin->config.steps - 1) \
	{ \
		for(int j = pkg->y1; j < pkg->y2; j++) \
		{ \
			TEMP *in_row = (TEMP*)plugin->accum + COMPONENTS * w * j; \
			TYPE *in_backup = (TYPE*)plugin->input->get_rows()[j]; \
			TYPE *out_row = (TYPE*)plugin->output->get_rows()[j]; \
			for(int k = 0; k < w; k++) \
			{ \
				*out_row++ = (*in_row++ * fraction) / 0x10000; \
				in_backup++; \
 \
				if(DO_YUV) \
				{ \
					*out_row++ = (*in_row++ * fraction) / 0x10000; \
					in_backup++; \
 \
					*out_row++ = (*in_row++ * fraction) / 0x10000; \
					in_backup++; \
				} \
				else \
				{ \
					*out_row++ = (*in_row++ * fraction) / 0x10000; \
					in_backup++; \
					*out_row++ = (*in_row++ * fraction) / 0x10000; \
					in_backup++; \
				} \
 \
				if(COMPONENTS == 4) \
				{ \
					*out_row++ = (*in_row++ * fraction) / 0x10000; \
					in_backup++; \
				} \
			} \
		} \
	} \
}

void MotionBlurUnit::process_package(LoadPackage *package)
{
	MotionBlurPackage *pkg = (MotionBlurPackage*)package;
	int h = plugin->output->get_h();
	int w = plugin->output->get_w();

	int fraction = 0x10000 / plugin->config.steps;
	for(int i = 0; i < plugin->config.steps; i++)
	{
		int *x_table = plugin->scale_x_table[i];
		int *y_table = plugin->scale_y_table[i];

		switch(plugin->input->get_color_model())
		{
			case BC_RGB_FLOAT:
				BLEND_LAYER(3, float, float, 1, 0)
				break;
			case BC_RGB888:
				BLEND_LAYER(3, uint8_t, int, 0xff, 0)
				break;
			case BC_RGBA_FLOAT:
				BLEND_LAYER(4, float, float, 1, 0)
				break;
			case BC_RGBA8888:
				BLEND_LAYER(4, uint8_t, int, 0xff, 0)
				break;
			case BC_RGB161616:
				BLEND_LAYER(3, uint16_t, int, 0xffff, 0)
				break;
			case BC_RGBA16161616:
				BLEND_LAYER(4, uint16_t, int, 0xffff, 0)
				break;
			case BC_YUV888:
				BLEND_LAYER(3, uint8_t, int, 0xff, 1)
				break;
			case BC_YUVA8888:
				BLEND_LAYER(4, uint8_t, int, 0xff, 1)
				break;
			case BC_YUV161616:
				BLEND_LAYER(3, uint16_t, int, 0xffff, 1)
				break;
			case BC_YUVA16161616:
				BLEND_LAYER(4, uint16_t, int, 0xffff, 1)
				break;
		}
	}
}






MotionBlurEngine::MotionBlurEngine(MotionBlurMain *plugin, 
	int total_clients, 
	int total_packages)
 : LoadServer(total_clients, total_packages)
{
	this->plugin = plugin;
}

void MotionBlurEngine::init_packages()
{
	for(int i = 0; i < get_total_packages(); i++)
	{
		MotionBlurPackage *package = (MotionBlurPackage*)get_package(i);
		package->y1 = plugin->output->get_h() * i / get_total_packages();
		package->y2 = plugin->output->get_h() * (i + 1) / get_total_packages();
	}
}

LoadClient* MotionBlurEngine::new_client()
{
	return new MotionBlurUnit(this, plugin);
}

LoadPackage* MotionBlurEngine::new_package()
{
	return new MotionBlurPackage;
}





