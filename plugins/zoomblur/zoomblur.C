
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

#define PLUGIN_TITLE N_("Zoom Blur")
#define PLUGIN_CLASS ZoomBlurMain
#define PLUGIN_CONFIG_CLASS ZoomBlurConfig
#define PLUGIN_THREAD_CLASS ZoomBlurThread
#define PLUGIN_GUI_CLASS ZoomBlurWindow

#include "pluginmacros.h"

#include <math.h>
#include <stdint.h>
#include <string.h>

#define GL_GLEXT_PROTOTYPES
#include "bchash.h"
#include "bcsignals.h"
#include "bcslider.h"
#include "bctitle.h"
#include "bctoggle.h"
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

class ZoomBlurEngine;


class ZoomBlurConfig
{
public:
	ZoomBlurConfig();

	int equivalent(ZoomBlurConfig &that);
	void copy_from(ZoomBlurConfig &that);
	void interpolate(ZoomBlurConfig &prev, 
		ZoomBlurConfig &next, 
		ptstime prev_pts,
		ptstime next_pts,
		ptstime current_pts);

	int x;
	int y;
	int radius;
	int steps;
	int r;
	int g;
	int b;
	int a;
	PLUGIN_CONFIG_CLASS_MEMBERS
};

class ZoomBlurSize : public BC_ISlider
{
public:
	ZoomBlurSize(ZoomBlurMain *plugin, 
		int x, 
		int y, 
		int *output,
		int min,
		int max);
	int handle_event();
	ZoomBlurMain *plugin;
	int *output;
};

class ZoomBlurToggle : public BC_CheckBox
{
public:
	ZoomBlurToggle(ZoomBlurMain *plugin, 
		int x, 
		int y, 
		int *output,
		char *string);
	int handle_event();
	ZoomBlurMain *plugin;
	int *output;
};

class ZoomBlurWindow : public PluginWindow
{
public:
	ZoomBlurWindow(ZoomBlurMain *plugin, int x, int y);
	~ZoomBlurWindow();

	void update();

	ZoomBlurSize *x, *y, *radius, *steps;
	ZoomBlurToggle *r, *g, *b, *a;
	PLUGIN_GUI_CLASS_MEMBERS
};


PLUGIN_THREAD_HEADER


// Output coords for a layer of blurring
// Used for OpenGL only
class ZoomBlurLayer
{
public:
	ZoomBlurLayer() {};
	float x1, y1, x2, y2;
};

class ZoomBlurMain : public PluginVClient
{
public:
	ZoomBlurMain(PluginServer *server);
	~ZoomBlurMain();

	void process_frame(VFrame *frame);
	void load_defaults();
	void save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void handle_opengl();

	PLUGIN_CLASS_MEMBERS

	void delete_tables();
	VFrame *input, *output, *temp;
	ZoomBlurEngine *engine;
	int **scale_y_table;
	int **scale_x_table;
	ZoomBlurLayer *layer_table;
	int table_entries;
	int need_reconfigure;
// The accumulation buffer is needed because 8 bits isn't precise enough
	unsigned char *accum;
};

class ZoomBlurPackage : public LoadPackage
{
public:
	ZoomBlurPackage();
	int y1, y2;
};

class ZoomBlurUnit : public LoadClient
{
public:
	ZoomBlurUnit(ZoomBlurEngine *server, ZoomBlurMain *plugin);
	void process_package(LoadPackage *package);
	ZoomBlurEngine *server;
	ZoomBlurMain *plugin;
};

class ZoomBlurEngine : public LoadServer
{
public:
	ZoomBlurEngine(ZoomBlurMain *plugin, 
		int total_clients, 
		int total_packages);
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
	ZoomBlurMain *plugin;
};


REGISTER_PLUGIN


ZoomBlurConfig::ZoomBlurConfig()
{
	x = 50;
	y = 50;
	radius = 10;
	steps = 10;
	r = 1;
	g = 1;
	b = 1;
	a = 1;
}

int ZoomBlurConfig::equivalent(ZoomBlurConfig &that)
{
	return 
		x == that.x &&
		y == that.y &&
		radius == that.radius &&
		steps == that.steps &&
		r == that.r &&
		g == that.g &&
		b == that.b &&
		a == that.a;
}

void ZoomBlurConfig::copy_from(ZoomBlurConfig &that)
{
	x = that.x;
	y = that.y;
	radius = that.radius;
	steps = that.steps;
	r = that.r;
	g = that.g;
	b = that.b;
	a = that.a;
}

void ZoomBlurConfig::interpolate(ZoomBlurConfig &prev, 
	ZoomBlurConfig &next, 
	ptstime prev_pts,
	ptstime next_pts,
	ptstime current_pts)
{
	PLUGIN_CONFIG_INTERPOLATE_MACRO
	this->x = (int)(prev.x * prev_scale + next.x * next_scale + 0.5);
	this->y = (int)(prev.y * prev_scale + next.y * next_scale + 0.5);
	this->radius = (int)(prev.radius * prev_scale + next.radius * next_scale + 0.5);
	this->steps = (int)(prev.steps * prev_scale + next.steps * next_scale + 0.5);
	r = prev.r;
	g = prev.g;
	b = prev.b;
	a = prev.a;
}

PLUGIN_THREAD_METHODS

ZoomBlurWindow::ZoomBlurWindow(ZoomBlurMain *plugin, int x, int y)
 : PluginWindow(plugin->gui_string, 
	x,
	y,
	230, 
	340)
{
	x = y = 10;

	add_subwindow(new BC_Title(x, y, _("X:")));
	y += 20;
	add_subwindow(this->x = new ZoomBlurSize(plugin, x, y, &plugin->config.x, 0, 100));
	y += 30;
	add_subwindow(new BC_Title(x, y, _("Y:")));
	y += 20;
	add_subwindow(this->y = new ZoomBlurSize(plugin, x, y, &plugin->config.y, 0, 100));
	y += 30;
	add_subwindow(new BC_Title(x, y, _("Radius:")));
	y += 20;
	add_subwindow(radius = new ZoomBlurSize(plugin, x, y, &plugin->config.radius, -100, 100));
	y += 30;
	add_subwindow(new BC_Title(x, y, _("Steps:")));
	y += 20;
	add_subwindow(steps = new ZoomBlurSize(plugin, x, y, &plugin->config.steps, 1, 100));
	y += 30;
	add_subwindow(r = new ZoomBlurToggle(plugin, x, y, &plugin->config.r, _("Red")));
	y += 30;
	add_subwindow(g = new ZoomBlurToggle(plugin, x, y, &plugin->config.g, _("Green")));
	y += 30;
	add_subwindow(b = new ZoomBlurToggle(plugin, x, y, &plugin->config.b, _("Blue")));
	y += 30;
	add_subwindow(a = new ZoomBlurToggle(plugin, x, y, &plugin->config.a, _("Alpha")));
	y += 30;
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

ZoomBlurWindow::~ZoomBlurWindow()
{
}

void ZoomBlurWindow::update()
{
	x->update(plugin->config.x);
	y->update(plugin->config.y);
	radius->update(plugin->config.radius);
	steps->update(plugin->config.steps);
	r->update(plugin->config.r);
	g->update(plugin->config.g);
	b->update(plugin->config.b);
	a->update(plugin->config.a);
}

ZoomBlurToggle::ZoomBlurToggle(ZoomBlurMain *plugin, 
	int x, 
	int y, 
	int *output, 
	char *string)
 : BC_CheckBox(x, y, *output, string)
{
	this->plugin = plugin;
	this->output = output;
}

int ZoomBlurToggle::handle_event()
{
	*output = get_value();
	plugin->send_configure_change();
	return 1;
}

ZoomBlurSize::ZoomBlurSize(ZoomBlurMain *plugin, 
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

int ZoomBlurSize::handle_event()
{
	*output = get_value();
	plugin->send_configure_change();
	return 1;
}

ZoomBlurMain::ZoomBlurMain(PluginServer *server)
 : PluginVClient(server)
{
	engine = 0;
	scale_x_table = 0;
	scale_y_table = 0;
	layer_table = 0;
	table_entries = 0;
	accum = 0;
	need_reconfigure = 1;
	temp = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

ZoomBlurMain::~ZoomBlurMain()
{
	if(engine) delete engine;
	delete_tables();
	if(accum) delete [] accum;
	if(temp) delete temp;
	PLUGIN_DESTRUCTOR_MACRO
}

PLUGIN_CLASS_METHODS

void ZoomBlurMain::delete_tables()
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
	scale_x_table = 0;
	scale_y_table = 0;
	layer_table = 0;
	table_entries = 0;
}

void ZoomBlurMain::process_frame(VFrame *frame)
{
	need_reconfigure |= load_configuration();

	get_frame(frame, get_use_opengl());

// Generate tables here.  The same table is used by many packages to render
// each horizontal stripe.  Need to cover the entire output range in  each
// table to avoid green borders
	if(need_reconfigure)
	{
		float w = frame->get_w();
		float h = frame->get_h();
		float center_x = (float)config.x / 100 * w;
		float center_y = (float)config.y / 100 * h;
		float radius = (float)(100 + config.radius) / 100;
		float min_w, min_h;
		float max_w, max_h;
		int steps = config.steps ? config.steps : 1;
		float min_x1;
		float min_y1;
		float min_x2;
		float min_y2;
		float max_x1;
		float max_y1;
		float max_x2;
		float max_y2;

		center_x = (center_x - w / 2) * (1.0 - radius) + w / 2;
		center_y = (center_y - h / 2) * (1.0 - radius) + h / 2;
		min_w = w * radius;
		min_h = h * radius;
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

// Dimensions of outermost rectangle
		delete_tables();
		table_entries = steps;
		scale_x_table = new int*[steps];
		scale_y_table = new int*[steps];
		layer_table = new ZoomBlurLayer[table_entries];

		for(int i = 0; i < steps; i++)
		{
			float fraction = (float)i / steps;
			float inv_fraction = 1.0 - fraction;
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

			layer_table[i].x1 = out_x1;
			layer_table[i].y1 = out_y1;
			layer_table[i].x2 = out_x2;
			layer_table[i].y2 = out_y2;

			for(int j = 0; j < h; j++)
			{
				y_table[j] = (int)((j - out_y1) * scale_y);
			}
			for(int j = 0; j < w; j++)
			{
				x_table[j] = (int)((j - out_x1) * scale_x);
			}
		}
		need_reconfigure = 0;
	}

	if(get_use_opengl())
	{
		run_opengl();
		return;
	}

	if(!engine) engine = new ZoomBlurEngine(this,
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

void ZoomBlurMain::load_defaults()
{
	defaults = load_defaults_file("zoomblur.rc");

	config.x = defaults->get("X", config.x);
	config.y = defaults->get("Y", config.y);
	config.radius = defaults->get("RADIUS", config.radius);
	config.steps = defaults->get("STEPS", config.steps);
	config.r = defaults->get("R", config.r);
	config.g = defaults->get("G", config.g);
	config.b = defaults->get("B", config.b);
	config.a = defaults->get("A", config.a);
}

void ZoomBlurMain::save_defaults()
{
	defaults->update("X", config.x);
	defaults->update("Y", config.y);
	defaults->update("RADIUS", config.radius);
	defaults->update("STEPS", config.steps);
	defaults->update("R", config.r);
	defaults->update("G", config.g);
	defaults->update("B", config.b);
	defaults->update("A", config.a);
	defaults->save();
}

void ZoomBlurMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_string(keyframe->data, MESSAGESIZE);
	output.tag.set_title("ZOOMBLUR");

	output.tag.set_property("X", config.x);
	output.tag.set_property("Y", config.y);
	output.tag.set_property("RADIUS", config.radius);
	output.tag.set_property("STEPS", config.steps);
	output.tag.set_property("R", config.r);
	output.tag.set_property("G", config.g);
	output.tag.set_property("B", config.b);
	output.tag.set_property("A", config.a);
	output.append_tag();
	output.tag.set_title("/ZOOMBLUR");
	output.append_tag();
	output.terminate_string();
}

void ZoomBlurMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->data, strlen(keyframe->data));

	while(!input.read_tag())
	{
		if(input.tag.title_is("ZOOMBLUR"))
		{
			config.x = input.tag.get_property("X", config.x);
			config.y = input.tag.get_property("Y", config.y);
			config.radius = input.tag.get_property("RADIUS", config.radius);
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

void ZoomBlurMain::handle_opengl()
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

		get_output()->draw_texture(0,
			0,
			get_output()->get_w(),
			get_output()->get_h(),
			layer_table[i].x1,
			get_output()->get_h() - layer_table[i].y1,
			layer_table[i].x2,
			get_output()->get_h() - layer_table[i].y2,
			1);

// Fill YUV black
		glDisable(GL_TEXTURE_2D);
		if(ColorModels::is_yuv(get_output()->get_color_model()))
		{
			glColor4f(config.r ? 0.0 : 0, 
				config.g ? 0.5 : 0, 
				config.b ? 0.5 : 0, 
				config.a ? 1.0 : 0);
			float center_x1 = 0.0;
			float center_x2 = get_output()->get_w();
			if(layer_table[i].x1 > 0)
			{
				center_x1 = layer_table[i].x1;
				draw_box(0, 0, layer_table[i].x1, -get_output()->get_h());
			}
			if(layer_table[i].x2 < get_output()->get_w())
			{
				center_x2 = layer_table[i].x2;
				draw_box(layer_table[i].x2, 0, get_output()->get_w(), -get_output()->get_h());
			}
			if(layer_table[i].y1 > 0)
			{
				draw_box(center_x1, 
					-get_output()->get_h(), 
					center_x2, 
					-get_output()->get_h() + layer_table[i].y1);
			}
			if(layer_table[i].y2 < get_output()->get_h())
			{
				draw_box(center_x1, 
					-get_output()->get_h() + layer_table[i].y2, 
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
	glReadBuffer(GL_BACK);
	glDisable(GL_TEXTURE_2D);
	glAccum(GL_RETURN, 1.0);

	glColor4f(1, 1, 1, 1);
	get_output()->set_opengl_state(VFrame::SCREEN);
	*/
#endif
}


ZoomBlurPackage::ZoomBlurPackage()
 : LoadPackage()
{
}

ZoomBlurUnit::ZoomBlurUnit(ZoomBlurEngine *server, 
	ZoomBlurMain *plugin)
 : LoadClient(server)
{
	this->plugin = plugin;
	this->server = server;
}


#define BLEND_LAYER(COMPONENTS, TYPE, TEMP_TYPE, MAX, DO_YUV) \
{ \
	const int chroma_offset = (DO_YUV ? ((MAX + 1) / 2) : 0); \
	for(int j = pkg->y1; j < pkg->y2; j++) \
	{ \
		TEMP_TYPE *out_row = (TEMP_TYPE*)plugin->accum + COMPONENTS * w * j; \
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
						*out_row++ += (TEMP_TYPE)in_row[in_offset + 1]; \
						*out_row++ += (TEMP_TYPE)in_row[in_offset + 2]; \
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
/* Copy just selected blurred channels to output and combine with original \
	unblurred channels */ \
	if(i == plugin->config.steps - 1) \
	{ \
		for(int j = pkg->y1; j < pkg->y2; j++) \
		{ \
			TEMP_TYPE *in_row = (TEMP_TYPE*)plugin->accum + COMPONENTS * w * j; \
			TYPE *in_backup = (TYPE*)plugin->input->get_rows()[j]; \
			TYPE *out_row = (TYPE*)plugin->output->get_rows()[j]; \
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
						*out_row++ = ((*in_row++ * fraction) / 0x10000); \
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
						*out_row++ = ((*in_row++ * fraction) / 0x10000); \
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

void ZoomBlurUnit::process_package(LoadPackage *package)
{
	ZoomBlurPackage *pkg = (ZoomBlurPackage*)package;
	int h = plugin->output->get_h();
	int w = plugin->output->get_w();
	int do_r = plugin->config.r;
	int do_g = plugin->config.g;
	int do_b = plugin->config.b;
	int do_a = plugin->config.a;

	int fraction = 0x10000 / plugin->config.steps;
	for(int i = 0; i < plugin->config.steps; i++)
	{
		int *x_table = plugin->scale_x_table[i];
		int *y_table = plugin->scale_y_table[i];

		switch(plugin->input->get_color_model())
		{
		case BC_RGB888:
			BLEND_LAYER(3, uint8_t, int, 0xff, 0)
			break;
		case BC_RGB_FLOAT:
			BLEND_LAYER(3, float, float, 1, 0)
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


ZoomBlurEngine::ZoomBlurEngine(ZoomBlurMain *plugin, 
	int total_clients, 
	int total_packages)
 : LoadServer(total_clients, total_packages)
{
	this->plugin = plugin;
}

void ZoomBlurEngine::init_packages()
{
	for(int i = 0; i < get_total_packages(); i++)
	{
		ZoomBlurPackage *package = (ZoomBlurPackage*)get_package(i);
		package->y1 = plugin->output->get_h() * i / get_total_packages();
		package->y2 = plugin->output->get_h() * (i + 1) / get_total_packages();
	}
}

LoadClient* ZoomBlurEngine::new_client()
{
	return new ZoomBlurUnit(this, plugin);
}

LoadPackage* ZoomBlurEngine::new_package()
{
	return new ZoomBlurPackage;
}
