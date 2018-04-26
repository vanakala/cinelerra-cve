
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

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME

#define PLUGIN_TITLE N_("Linear Blur")
#define PLUGIN_CLASS LinearBlurMain
#define PLUGIN_CONFIG_CLASS LinearBlurConfig
#define PLUGIN_THREAD_CLASS LinearBlurThread
#define PLUGIN_GUI_CLASS LinearBlurWindow

#include "pluginmacros.h"

#include <math.h>
#include <stdint.h>
#include <string.h>

#define GL_GLEXT_PROTOTYPES

#include "bchash.h"
#include "bcslider.h"
#include "bctitle.h"
#include "clip.h"
#include "filexml.h"
#include "keyframe.h"
#include "language.h"
#include "loadbalance.h"
#include "picon_png.h"
#include "pluginvclient.h"
#include "pluginwindow.h"
#include "vframe.h"

#ifdef HAVE_GL
#include <GL/gl.h>
#include <GL/glx.h>
#endif


class LinearBlurEngine;

class LinearBlurConfig
{
public:
	LinearBlurConfig();

	int equivalent(LinearBlurConfig &that);
	void copy_from(LinearBlurConfig &that);
	void interpolate(LinearBlurConfig &prev, 
		LinearBlurConfig &next, 
		ptstime prev_pts,
		ptstime next_pts,
		ptstime current_pts);

	int radius;
	int steps;
	int angle;
	int r;
	int g;
	int b;
	int a;
	PLUGIN_CONFIG_CLASS_MEMBERS
};


class LinearBlurSize : public BC_ISlider
{
public:
	LinearBlurSize(LinearBlurMain *plugin, 
		int x, 
		int y, 
		int *output,
		int min,
		int max);
	int handle_event();
	LinearBlurMain *plugin;
	int *output;
};

class LinearBlurToggle : public BC_CheckBox
{
public:
	LinearBlurToggle(LinearBlurMain *plugin, 
		int x, 
		int y, 
		int *output,
		char *string);
	int handle_event();
	LinearBlurMain *plugin;
	int *output;
};

class LinearBlurWindow : public PluginWindow
{
public:
	LinearBlurWindow(LinearBlurMain *plugin, int x, int y);
	~LinearBlurWindow();

	void update();

	LinearBlurSize *angle, *steps, *radius;
	LinearBlurToggle *r, *g, *b, *a;
	PLUGIN_GUI_CLASS_MEMBERS
};


PLUGIN_THREAD_HEADER


// Output coords for a layer of blurring
// Used for OpenGL only
class LinearBlurLayer
{
public:
	LinearBlurLayer() {};
	int x, y;
};

class LinearBlurMain : public PluginVClient
{
public:
	LinearBlurMain(PluginServer *server);
	~LinearBlurMain();

	void process_frame(VFrame *frame);
	void load_defaults();
	void save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void handle_opengl();

	PLUGIN_CLASS_MEMBERS

	void delete_tables();
	VFrame *input, *output, *temp;
	LinearBlurEngine *engine;
	int **scale_y_table;
	int **scale_x_table;
	LinearBlurLayer *layer_table;
	int table_entries;
	int need_reconfigure;
// The accumulation buffer is needed because 8 bits isn't precise enough
	unsigned char *accum;
};

class LinearBlurPackage : public LoadPackage
{
public:
	LinearBlurPackage();
	int y1, y2;
};

class LinearBlurUnit : public LoadClient
{
public:
	LinearBlurUnit(LinearBlurEngine *server, LinearBlurMain *plugin);
	void process_package(LoadPackage *package);
	LinearBlurEngine *server;
	LinearBlurMain *plugin;
};

class LinearBlurEngine : public LoadServer
{
public:
	LinearBlurEngine(LinearBlurMain *plugin, 
		int total_clients, 
		int total_packages);
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
	LinearBlurMain *plugin;
};


REGISTER_PLUGIN

LinearBlurConfig::LinearBlurConfig()
{
	radius = 10;
	angle = 0;
	steps = 10;
	r = 1;
	g = 1;
	b = 1;
	a = 1;
}

int LinearBlurConfig::equivalent(LinearBlurConfig &that)
{
	return 
		radius == that.radius &&
		angle == that.angle &&
		steps == that.steps &&
		r == that.r &&
		g == that.g &&
		b == that.b &&
		a == that.a;
}

void LinearBlurConfig::copy_from(LinearBlurConfig &that)
{
	radius = that.radius;
	angle = that.angle;
	steps = that.steps;
	r = that.r;
	g = that.g;
	b = that.b;
	a = that.a;
}

void LinearBlurConfig::interpolate(LinearBlurConfig &prev, 
	LinearBlurConfig &next, 
	ptstime prev_pts,
	ptstime next_pts,
	ptstime current_pts)
{
	PLUGIN_CONFIG_INTERPOLATE_MACRO

	this->radius = (int)(prev.radius * prev_scale + next.radius * next_scale + 0.5);
	this->angle = (int)(prev.angle * prev_scale + next.angle * next_scale + 0.5);
	this->steps = (int)(prev.steps * prev_scale + next.steps * next_scale + 0.5);
	r = prev.r;
	g = prev.g;
	b = prev.b;
	a = prev.a;
}

PLUGIN_THREAD_METHODS

LinearBlurWindow::LinearBlurWindow(LinearBlurMain *plugin, int x, int y)
 : PluginWindow(plugin->gui_string, 
	x,
	y,
	230, 
	290)
{
	x = y = 10;

	add_subwindow(new BC_Title(x, y, _("Length:")));
	y += 20;
	add_subwindow(radius = new LinearBlurSize(plugin, x, y, &plugin->config.radius, 0, 100));
	y += 30;
	add_subwindow(new BC_Title(x, y, _("Angle:")));
	y += 20;
	add_subwindow(angle = new LinearBlurSize(plugin, x, y, &plugin->config.angle, -180, 180));
	y += 30;
	add_subwindow(new BC_Title(x, y, _("Steps:")));
	y += 20;
	add_subwindow(steps = new LinearBlurSize(plugin, x, y, &plugin->config.steps, 1, 200));
	y += 30;
	add_subwindow(r = new LinearBlurToggle(plugin, x, y, &plugin->config.r, _("Red")));
	y += 30;
	add_subwindow(g = new LinearBlurToggle(plugin, x, y, &plugin->config.g, _("Green")));
	y += 30;
	add_subwindow(b = new LinearBlurToggle(plugin, x, y, &plugin->config.b, _("Blue")));
	y += 30;
	add_subwindow(a = new LinearBlurToggle(plugin, x, y, &plugin->config.a, _("Alpha")));
	y += 30;
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

LinearBlurWindow::~LinearBlurWindow()
{
}

void LinearBlurWindow::update()
{
	radius->update(plugin->config.radius);
	angle->update(plugin->config.angle);
	steps->update(plugin->config.steps);
	r->update(plugin->config.r);
	g->update(plugin->config.g);
	b->update(plugin->config.b);
	a->update(plugin->config.a);
}

LinearBlurToggle::LinearBlurToggle(LinearBlurMain *plugin, 
	int x, 
	int y, 
	int *output, 
	char *string)
 : BC_CheckBox(x, y, *output, string)
{
	this->plugin = plugin;
	this->output = output;
}

int LinearBlurToggle::handle_event()
{
	*output = get_value();
	plugin->send_configure_change();
	return 1;
}

LinearBlurSize::LinearBlurSize(LinearBlurMain *plugin, 
	int x, 
	int y, 
	int *output,
	int min,
	int max)
 : BC_ISlider(x, y, 0, 200, 200, min, max, *output)
{
	this->plugin = plugin;
	this->output = output;
}

int LinearBlurSize::handle_event()
{
	*output = get_value();
	plugin->send_configure_change();
	return 1;
}

PLUGIN_CLASS_METHODS

LinearBlurMain::LinearBlurMain(PluginServer *server)
 : PluginVClient(server)
{
	engine = 0;
	scale_x_table = 0;
	scale_y_table = 0;
	table_entries = 0;
	accum = 0;
	need_reconfigure = 1;
	temp = 0;
	layer_table = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

LinearBlurMain::~LinearBlurMain()
{
	if(engine) delete engine;
	delete_tables();
	if(accum) delete [] accum;
	if(temp) delete temp;
	PLUGIN_DESTRUCTOR_MACRO
}

void LinearBlurMain::delete_tables()
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
	delete [] layer_table;
	layer_table = 0;
	scale_x_table = 0;
	scale_y_table = 0;
	table_entries = 0;
}

void LinearBlurMain::process_frame(VFrame *frame)
{
	need_reconfigure |= load_configuration();

	get_frame(frame, get_use_opengl());
// Generate tables here.  The same table is used by many packages to render
// each horizontal stripe.  Need to cover the entire output range in  each
// table to avoid green borders
	if(need_reconfigure)
	{
		int w = frame->get_w();
		int h = frame->get_h();
		int x_offset;
		int y_offset;
		int angle = config.angle;
		int radius = config.radius * MIN(w, h) / 100;

		while(angle < 0) angle += 360;
		switch(angle)
		{
		case 0:
		case 360:
			x_offset = radius;
			y_offset = 0;
			break;
		case 90:
			x_offset = 0;
			y_offset = radius;
			break;
		case 180:
			x_offset = radius;
			y_offset = 0;
			break;
		case 270:
			x_offset = 0;
			y_offset = radius;
			break;
		default:
			y_offset = (int)(sin((float)config.angle / 360 * 2 * M_PI) * radius);
			x_offset = (int)(cos((float)config.angle / 360 * 2 * M_PI) * radius);
			break;
		}

		delete_tables();
		scale_x_table = new int*[config.steps];
		scale_y_table = new int*[config.steps];
		table_entries = config.steps;
		layer_table = new LinearBlurLayer[table_entries];

		for(int i = 0; i < config.steps; i++)
		{
			float fraction = (float)(i - config.steps / 2) / config.steps;
			int x = (int)(fraction * x_offset);
			int y = (int)(fraction * y_offset);

			int *x_table;
			int *y_table;
			scale_y_table[i] = y_table = new int[h];
			scale_x_table[i] = x_table = new int[w];

			layer_table[i].x = x;
			layer_table[i].y = y;
			for(int j = 0; j < h; j++)
			{
				y_table[j] = j + y;
				CLAMP(y_table[j], 0, h - 1);
			}
			for(int j = 0; j < w; j++)
			{
				x_table[j] = j + x;
				CLAMP(x_table[j], 0, w - 1);
			}
		}
		need_reconfigure = 0;
	}

	if(get_use_opengl())
	{
		run_opengl();
		return;
	}

	if(!engine) engine = new LinearBlurEngine(this,
		get_project_smp() + 1,
		get_project_smp() + 1);
	if(!accum) accum = new unsigned char[frame->get_w() * 
		frame->get_h() *
		ColorModels::components(frame->get_color_model()) *
		MAX(sizeof(int), sizeof(float))];

	this->input = frame;
	this->output = frame;

	if(!temp) temp = new VFrame(0,
		frame->get_w(),
		frame->get_h(),
		frame->get_color_model());
	temp->copy_from(frame);
	this->input = temp;

	memset(accum, 0,
		frame->get_w() * 
		frame->get_h() * 
		ColorModels::components(frame->get_color_model()) *
		MAX(sizeof(int), sizeof(float)));
	engine->process_packages();
}

void LinearBlurMain::load_defaults()
{
	defaults = load_defaults_file("linearblur.rc");

	config.radius = defaults->get("RADIUS", config.radius);
	config.angle = defaults->get("ANGLE", config.angle);
	config.steps = defaults->get("STEPS", config.steps);
	config.r = defaults->get("R", config.r);
	config.g = defaults->get("G", config.g);
	config.b = defaults->get("B", config.b);
	config.a = defaults->get("A", config.a);
}

void LinearBlurMain::save_defaults()
{
	defaults->update("RADIUS", config.radius);
	defaults->update("ANGLE", config.angle);
	defaults->update("STEPS", config.steps);
	defaults->update("R", config.r);
	defaults->update("G", config.g);
	defaults->update("B", config.b);
	defaults->update("A", config.a);
	defaults->save();
}

void LinearBlurMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_string(keyframe->data, MESSAGESIZE);
	output.tag.set_title("LINEARBLUR");

	output.tag.set_property("RADIUS", config.radius);
	output.tag.set_property("ANGLE", config.angle);
	output.tag.set_property("STEPS", config.steps);
	output.tag.set_property("R", config.r);
	output.tag.set_property("G", config.g);
	output.tag.set_property("B", config.b);
	output.tag.set_property("A", config.a);
	output.append_tag();
	output.tag.set_title("/LINEARBLUR");
	output.append_tag();
	output.terminate_string();
}

void LinearBlurMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->data, strlen(keyframe->data));

	while(!input.read_tag())
	{
		if(input.tag.title_is("LINEARBLUR"))
		{
			config.radius = input.tag.get_property("RADIUS", config.radius);
			config.angle = input.tag.get_property("ANGLE", config.angle);
			config.steps = input.tag.get_property("STEPS", config.steps);
			config.r = input.tag.get_property("R", config.r);
			config.g = input.tag.get_property("G", config.g);
			config.b = input.tag.get_property("B", config.b);
			config.a = input.tag.get_property("A", config.a);
		}
	}
}

#ifdef HAVE_GL
static void draw_box(float x1, float y1, float x2, float y2)
{
	glBegin(GL_QUADS);
	glVertex3f(x1, y1, 0.0);
	glVertex3f(x2, y1, 0.0);
	glVertex3f(x2, y2, 0.0);
	glVertex3f(x1, y2, 0.0);
	glEnd();
}
#endif

void LinearBlurMain::handle_opengl()
{
#ifdef HAVE_GL
/* FIXIT
	get_output()->to_texture();
	get_output()->enable_opengl();
	get_output()->init_screen();
	get_output()->bind_texture(0);

	int is_yuv = ColorModels::is_yuv(get_output()->get_color_model());
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT);

// Draw unselected channels
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	glDrawBuffer(GL_BACK);
	if(!config.r || !config.g || !config.b || !config.a)
	{
		glColor4f(config.r ? 0 : 1, 
			config.g ? 0 : 1, 
			config.b ? 0 : 1, 
			config.a ? 0 : 1);
		get_output()->draw_texture();
	}
	glAccum(GL_LOAD, 1.0);

// Blur selected channels
	float fraction = 1.0 / config.steps;
	for(int i = 0; i < config.steps; i++)
	{
		glClear(GL_COLOR_BUFFER_BIT);
		glColor4f(config.r ? 1 : 0, 
			config.g ? 1 : 0, 
			config.b ? 1 : 0, 
			config.a ? 1 : 0);

		int w = get_output()->get_w();
		int h = get_output()->get_h();
		get_output()->draw_texture(0,
			0,
			w,
			h,
			layer_table[i].x,
			get_output()->get_h() - layer_table[i].y,
			layer_table[i].x + w,
			get_output()->get_h() - layer_table[i].y - h,
			1);

// Fill YUV black
		glDisable(GL_TEXTURE_2D);
		if(is_yuv)
		{
			glColor4f(config.r ? 0.0 : 0, 
				config.g ? 0.5 : 0, 
				config.b ? 0.5 : 0, 
				config.a ? 1.0 : 0);
			float center_x1 = 0.0;
			float center_x2 = get_output()->get_w();
			float project_x1 = layer_table[i].x;
			float project_x2 = layer_table[i].x + get_output()->get_w();
			float project_y1 = layer_table[i].y;
			float project_y2 = layer_table[i].y + get_output()->get_h();
			if(project_x1 > 0)
			{
				center_x1 = project_x1;
				draw_box(0, 0, project_x1, -get_output()->get_h());
			}
			if(project_x2 < get_output()->get_w())
			{
				center_x2 = project_x2;
				draw_box(project_x2, 0, get_output()->get_w(), -get_output()->get_h());
			}
			if(project_y1 > 0)
			{
				draw_box(center_x1, 
					-get_output()->get_h(), 
					center_x2, 
					-get_output()->get_h() + project_y1);
			}
			if(project_y2 < get_output()->get_h())
			{
				draw_box(center_x1, 
					-get_output()->get_h() + project_y2, 
					center_x2, 
					0);
			}
		}

		glAccum(GL_ACCUM, fraction);
		glEnable(GL_TEXTURE_2D);
		glColor4f(config.r ? 1 : 0, 
			config.g ? 1 : 0, 
			config.b ? 1 : 0, 
			config.a ? 1 : 0);
	}

	glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);
	glReadBuffer(GL_BACK);
	glAccum(GL_RETURN, 1.0);

	glColor4f(1, 1, 1, 1);
	get_output()->set_opengl_state(VFrame::SCREEN);
	*/
#endif
}

LinearBlurPackage::LinearBlurPackage()
 : LoadPackage()
{
}

LinearBlurUnit::LinearBlurUnit(LinearBlurEngine *server, 
	LinearBlurMain *plugin)
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
		TYPE *in_row = (TYPE*)plugin->input->get_row_ptr(in_y); \
		for(int k = 0; k < w; k++) \
		{ \
			int in_x = x_table[k]; \
/* Blend pixel */ \
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
	} \
 \
/* Copy to output */ \
	if(i == plugin->config.steps - 1) \
	{ \
		for(int j = pkg->y1; j < pkg->y2; j++) \
		{ \
			TEMP *in_row = (TEMP*)plugin->accum + COMPONENTS * w * j; \
			TYPE *in_backup = (TYPE*)plugin->input->get_row_ptr(j); \
			TYPE *out_row = (TYPE*)plugin->output->get_row_ptr(j); \
			for(int k = 0; k < w; k++) \
			{ \
				if(do_r) \
				{ \
					*out_row++ = (*in_row++ * fraction) / 0x10000; \
					in_backup++; \
				} \
				else \
				{ \
					*out_row++ = *in_backup++; \
					in_row++; \
				} \
 \
				if(DO_YUV) \
				{ \
					if(do_g) \
					{ \
						*out_row++ = (*in_row++ * fraction) / 0x10000; \
						in_backup++; \
					} \
					else \
					{ \
						*out_row++ = *in_backup++; \
						in_row++; \
					} \
 \
					if(do_b) \
					{ \
						*out_row++ = (*in_row++ * fraction) / 0x10000; \
						in_backup++; \
					} \
					else \
					{ \
						*out_row++ = *in_backup++; \
						in_row++; \
					} \
				} \
				else \
				{ \
					if(do_g) \
					{ \
						*out_row++ = (*in_row++ * fraction) / 0x10000; \
						in_backup++; \
					} \
					else \
					{ \
						*out_row++ = *in_backup++; \
						in_row++; \
					} \
 \
					if(do_b) \
					{ \
						*out_row++ = (*in_row++ * fraction) / 0x10000; \
						in_backup++; \
					} \
					else \
					{ \
						*out_row++ = *in_backup++; \
						in_row++; \
					} \
				} \
 \
				if(COMPONENTS == 4) \
				{ \
					if(do_a) \
					{ \
						*out_row++ = (*in_row++ * fraction) / 0x10000; \
						in_backup++; \
					} \
					else \
					{ \
						*out_row++ = *in_backup++; \
						in_row++; \
					} \
				} \
			} \
		} \
	} \
}

void LinearBlurUnit::process_package(LoadPackage *package)
{
	LinearBlurPackage *pkg = (LinearBlurPackage*)package;
	int h = plugin->output->get_h();
	int w = plugin->output->get_w();

	int fraction = 0x10000 / plugin->config.steps;
	int do_r = plugin->config.r;
	int do_g = plugin->config.g;
	int do_b = plugin->config.b;
	int do_a = plugin->config.a;
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
		case BC_AYUV16161616:
			for(int j = pkg->y1; j < pkg->y2; j++)
			{
				int *out_row = (int*)plugin->accum + 4 * w * j;
				int in_y = y_table[j];

				// Blend image
				uint16_t *in_row = (uint16_t*)plugin->input->get_row_ptr(in_y); \
				for(int k = 0; k < w; k++)
				{
					int in_x = x_table[k];
					// Blend pixel
					int in_offset = in_x * 4;
					*out_row++ += in_row[in_offset];
					*out_row++ += in_row[in_offset + 1];
					*out_row++ += in_row[in_offset + 2];
					*out_row++ += in_row[in_offset + 3];
				}
			}

			// Copy to output
			if(i == plugin->config.steps - 1)
			{
				for(int j = pkg->y1; j < pkg->y2; j++)
				{
					int *in_row = (int*)plugin->accum + 4 * w * j;
					uint16_t *in_backup = (uint16_t*)plugin->input->get_row_ptr(j);
					uint16_t *out_row = (uint16_t*)plugin->output->get_row_ptr(j);
					for(int k = 0; k < w; k++)
					{
						if(do_a)
						{
							*out_row++ = (*in_row++ * fraction) / 0x10000;
							in_backup++;
						}
						else
						{
							*out_row++ = *in_backup++;
							in_row++;
						}

						if(do_r)
						{
							*out_row++ = (*in_row++ * fraction) / 0x10000;
							in_backup++;
						}
						else
						{
							*out_row++ = *in_backup++;
							in_row++;
						}

						if(do_g)
						{
							*out_row++ = (*in_row++ * fraction) / 0x10000;
							in_backup++;
						}
						else
						{
							*out_row++ = *in_backup++;
							in_row++;
						}

						if(do_b)
						{
							*out_row++ = (*in_row++ * fraction) / 0x10000;
							in_backup++;
						}
						else
						{
							*out_row++ = *in_backup++;
							in_row++;
						}
					}
				}
			}
			break;
		}
	}
}

LinearBlurEngine::LinearBlurEngine(LinearBlurMain *plugin, 
	int total_clients, 
	int total_packages)
 : LoadServer(total_clients, total_packages)
{
	this->plugin = plugin;
}

void LinearBlurEngine::init_packages()
{
	for(int i = 0; i < get_total_packages(); i++)
	{
		LinearBlurPackage *package = (LinearBlurPackage*)get_package(i);
		package->y1 = plugin->output->get_h() * i / get_total_packages();
		package->y2 = plugin->output->get_h() * (i + 1) / get_total_packages();
	}
}

LoadClient* LinearBlurEngine::new_client()
{
	return new LinearBlurUnit(this, plugin);
}

LoadPackage* LinearBlurEngine::new_package()
{
	return new LinearBlurPackage;
}
