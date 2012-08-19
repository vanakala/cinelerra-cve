/*
 * C41 plugin for Cinelerra
 * Copyright (C) 2011 Florent Delannoy <florent at plui dot es>
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

#define PLUGIN_TITLE N_("C41")
#define PLUGIN_CLASS C41Effect
#define PLUGIN_CONFIG_CLASS C41Config
#define PLUGIN_THREAD_CLASS C41Thread
#define PLUGIN_GUI_CLASS C41Window

#include "pluginmacros.h"

#include "clip.h"
#include "bchash.h"
#include "filexml.h"
#include "guicast.h"
#include "language.h"
#include "picon_png.h"
#include "plugincolors.h"
#include "pluginvclient.h"
#include "vframe.h"

// C41_FAST_POW increases processing speed more than 10 times
// Depending on gcc version, used optimizations and cpu C41_FAST_POW may not work
// Test some samples after you have enabled it
// #define C41_FAST_POW

#if defined(C41_FAST_POW)
#define C41_POW_FUNC myPow
#else
#define C41_POW_FUNC powf
#endif

// Shave the image in order to avoid black borders
// Tolerance default: 5%, i.e. 0.05
#define TOLERANCE 0.05
#define SKIP_ROW if(i < (TOLERANCE * frame_h) || i > ((1 - TOLERANCE) * frame_h)) continue
#define SKIP_COL if(j < (TOLERANCE * frame_w) || j > ((1-TOLERANCE) * frame_w)) continue


#include <stdint.h>
#include <string.h>

struct magic
{
	float min_r;
	float min_g;
	float min_b;
	float light;
	float gamma_g;
	float gamma_b;
};

class C41Config
{
public:
	C41Config();

	void copy_from(C41Config &src);
	int equivalent(C41Config &src);
	void interpolate(C41Config &prev,
			C41Config &next,
			ptstime prev_pts,
			ptstime next_pts,
			ptstime current_pts);

	int active;
	int compute_magic;
	float fix_min_r;
	float fix_min_g;
	float fix_min_b;
	float fix_light;
	float fix_gamma_g;
	float fix_gamma_b;
	PLUGIN_CONFIG_CLASS_MEMBERS
};

class C41Enable : public BC_CheckBox
{
public:
	C41Enable(C41Effect *plugin, int *output, int x, int y, const char *text);
	int handle_event();

	C41Effect *plugin;
	int *output;
};

class C41TextBox : public BC_TextBox
{
public:
	C41TextBox(C41Effect *plugin, float *value, int x, int y);
	int handle_event();

	C41Effect *plugin;
	float *boxValue;
};

class C41Button : public BC_GenericButton
{
public:
	C41Button(C41Effect *plugin, C41Window *window, int x, int y);
	int handle_event();

	C41Effect *plugin;
	C41Window *window;
	float *boxValue;
};

class C41Window : public BC_Window
{
public:
	C41Window(C41Effect *plugin, int x, int y);

	void update();
	void update_magic();

	C41Enable *active;
	C41Enable *compute_magic;
	BC_Title *min_r;
	BC_Title *min_g;
	BC_Title *min_b;
	BC_Title *light;
	BC_Title *gamma_g;
	BC_Title *gamma_b;
	C41TextBox *fix_min_r;
	C41TextBox *fix_min_g;
	C41TextBox *fix_min_b;
	C41TextBox *fix_light;
	C41TextBox *fix_gamma_g;
	C41TextBox *fix_gamma_b;
	C41Button *lock;
	PLUGIN_GUI_CLASS_MEMBERS
};

PLUGIN_THREAD_HEADER

class C41Effect : public PluginVClient
{
public:
	C41Effect(PluginServer *server);
	~C41Effect();

	void process_frame(VFrame *frame);
	void load_defaults();
	void save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void render_gui(void* data);
#if defined(C41_FAST_POW)
	float myLog2(float i);
	float myPow2(float i);
	float myPow(float a, float b);
#endif
	VFrame* tmp_frame;
	VFrame* blurry_frame;
	struct magic values;
	int64_t time_sum;
	PLUGIN_CLASS_MEMBERS
};


REGISTER_PLUGIN


// C41Config
C41Config::C41Config()
{
	active = 0;
	compute_magic = 0;

	fix_min_r = fix_min_g = fix_min_b = fix_light = fix_gamma_g = fix_gamma_b = 0.;
}

void C41Config::copy_from(C41Config &src)
{
	active = src.active;
	compute_magic = src.compute_magic;

	fix_min_r = src.fix_min_r;
	fix_min_g = src.fix_min_g;
	fix_min_b = src.fix_min_b;
	fix_light = src.fix_light;
	fix_gamma_g = src.fix_gamma_g;
	fix_gamma_b = src.fix_gamma_b;
}

int C41Config::equivalent(C41Config &src)
{
	return (active == src.active &&
		compute_magic == src.compute_magic &&
		EQUIV(fix_min_r, src.fix_min_r) &&
		EQUIV(fix_min_g, src.fix_min_g) &&
		EQUIV(fix_min_b, src.fix_min_b) &&
		EQUIV(fix_light, src.fix_light) &&
		EQUIV(fix_gamma_g, src.fix_gamma_g) &&
		EQUIV(fix_gamma_b, src.fix_gamma_b));
}

void C41Config::interpolate(C41Config &prev,
		C41Config &next,
		ptstime prev_pts,
		ptstime next_pts,
		ptstime current_pts)
{
	PLUGIN_CONFIG_INTERPOLATE_MACRO
	active = prev.active;
	compute_magic = prev.compute_magic;

	fix_min_r = prev.fix_min_r * prev_scale + next.fix_min_r * next_scale;
	fix_min_g = prev.fix_min_g * prev_scale + next.fix_min_g * next_scale;
	fix_min_b = prev.fix_min_b * prev_scale + next.fix_min_b * next_scale;
	fix_light = prev.fix_light * prev_scale + next.fix_light * next_scale;
	fix_gamma_g = prev.fix_gamma_g * prev_scale + next.fix_gamma_g * next_scale;
	fix_gamma_b = prev.fix_gamma_b * prev_scale + next.fix_gamma_b * next_scale;
}

// C41Enable
C41Enable::C41Enable(C41Effect *plugin, int *output, int x, int y, const char *text)
 : BC_CheckBox(x, y, *output, text)
{
	this->plugin = plugin;
	this->output = output;
}

int C41Enable::handle_event()
{
	*output = get_value();
	plugin->send_configure_change();
	return 1;
}

// C41TextBox
C41TextBox::C41TextBox(C41Effect *plugin, float *value, int x, int y)
 : BC_TextBox(x, y, 160, 1, *value)
{
	this->plugin = plugin;
	this->boxValue = value;
}

int C41TextBox::handle_event()
{
	*boxValue = atof(get_text());
	plugin->send_configure_change();
	return 1;
}


// C41Button
C41Button::C41Button(C41Effect *plugin, C41Window *window, int x, int y)
 : BC_GenericButton(x, y, "Lock parameters")
{
	this->plugin = plugin;
	this->window = window;
}

int C41Button::handle_event()
{
	plugin->config.fix_min_r = plugin->values.min_r;
	plugin->config.fix_min_g = plugin->values.min_g;
	plugin->config.fix_min_b = plugin->values.min_b;
	plugin->config.fix_light = plugin->values.light;
	plugin->config.fix_gamma_g = plugin->values.gamma_g;
	plugin->config.fix_gamma_b = plugin->values.gamma_b;

	window->update();

	plugin->send_configure_change();
	return 1;
}

PLUGIN_THREAD_METHODS

// C41Window
C41Window::C41Window(C41Effect *plugin, int x, int y)
 : BC_Window(plugin->gui_string, x, y, 250, 620, 250, 620, 1, 0, 1)
{
	x = y = 10;

	add_subwindow(active = new C41Enable(plugin, &plugin->config.active, x, y, _("Activate processing")));
	y += 40;

	add_subwindow(compute_magic = new C41Enable(plugin, &plugin->config.compute_magic, x, y, _("Compute negfix values")));
	y += 20;

	add_subwindow(new BC_Title(x + 20, y, _("(uncheck for faster rendering)")));
	y += 40;

	add_subwindow(new BC_Title(x, y, _("Computed negfix values:")));
	y += 30;

	add_subwindow(new BC_Title(x, y, _("Min R:")));
	add_subwindow(min_r = new BC_Title(x + 80, y, "0.0000"));
	y += 30;

	add_subwindow(new BC_Title(x, y, _("Min G:")));
	add_subwindow(min_g = new BC_Title(x + 80, y, "0.0000"));
	y += 30;

	add_subwindow(new BC_Title(x, y, _("Min B:")));
	add_subwindow(min_b = new BC_Title(x + 80, y, "0.0000"));
	y += 30;

	add_subwindow(new BC_Title(x, y, _("Light:")));
	add_subwindow(light = new BC_Title(x + 80, y, "0.0000"));
	y += 30;

	add_subwindow(new BC_Title(x, y, _("Gamma G:")));
	add_subwindow(gamma_g = new BC_Title(x + 80, y, "0.0000"));
	y += 30;

	add_subwindow(new BC_Title(x, y, _("Gamma B:")));
	add_subwindow(gamma_b = new BC_Title(x + 80, y, "0.0000"));
	y += 30;

	y += 30;
	add_subwindow(lock = new C41Button(plugin, this, x, y));
	y += 30;

	y += 20;
	add_subwindow(new BC_Title(x, y, _("negfix values to apply:")));
	y += 30;

	add_subwindow(new BC_Title(x, y, _("Min R:")));
	add_subwindow(fix_min_r = new C41TextBox(plugin, &plugin->config.fix_min_r, x + 80, y));
	y += 30;

	add_subwindow(new BC_Title(x, y, _("Min G:")));
	add_subwindow(fix_min_g = new C41TextBox(plugin, &plugin->config.fix_min_g, x + 80, y));
	y += 30;

	add_subwindow(new BC_Title(x, y, _("Min B:")));
	add_subwindow(fix_min_b = new C41TextBox(plugin, &plugin->config.fix_min_b, x + 80, y));
	y += 30;

	add_subwindow(new BC_Title(x, y, _("Light:")));
	add_subwindow(fix_light = new C41TextBox(plugin, &plugin->config.fix_light, x + 80, y));
	y += 30;

	add_subwindow(new BC_Title(x, y, _("Gamma G:")));
	add_subwindow(fix_gamma_g = new C41TextBox(plugin, &plugin->config.fix_gamma_g, x + 80, y));
	y += 30;

	add_subwindow(new BC_Title(x, y, _("Gamma B:")));
	add_subwindow(fix_gamma_b = new C41TextBox(plugin, &plugin->config.fix_gamma_b, x + 80, y));
	y += 30;
	PLUGIN_GUI_CONSTRUCTOR_MACRO
	update_magic();
}

void C41Window::update()
{
	// Updating the GUI itself
	active->update(plugin->config.active);
	compute_magic->update(plugin->config.compute_magic);

	fix_min_r->update(plugin->config.fix_min_r);
	fix_min_g->update(plugin->config.fix_min_g);
	fix_min_b->update(plugin->config.fix_min_b);
	fix_light->update(plugin->config.fix_light);
	fix_gamma_g->update(plugin->config.fix_gamma_g);
	fix_gamma_b->update(plugin->config.fix_gamma_b);
	update_magic();
}

void C41Window::update_magic()
{
	min_r->update(plugin->values.min_r);
	min_g->update(plugin->values.min_g);
	min_b->update(plugin->values.min_b);
	light->update(plugin->values.light);
	gamma_g->update(plugin->values.gamma_g);
	gamma_b->update(plugin->values.gamma_b);
}


// C41Effect
C41Effect::C41Effect(PluginServer *server)
: PluginVClient(server)
{
	tmp_frame = 0;
	blurry_frame = 0;
	memset(&values, 0, sizeof(values));
	PLUGIN_CONSTRUCTOR_MACRO
}

C41Effect::~C41Effect()
{
	if(tmp_frame)
		delete tmp_frame;
	if(blurry_frame)
		delete blurry_frame;
	PLUGIN_DESTRUCTOR_MACRO
}

PLUGIN_CLASS_METHODS

void C41Effect::render_gui(void* data)
{
	// Updating values computed by process_frame
	struct magic *vp = (struct magic *)data;
	values = *vp;

	if(thread)
		thread->window->update_magic();
}

void C41Effect::load_defaults()
{
	defaults = load_defaults_file("C41.rc");
	config.active = defaults->get("ACTIVE", config.active);
	config.compute_magic = defaults->get("COMPUTE_MAGIC", config.compute_magic);
	config.fix_min_r = defaults->get("FIX_MIN_R", config.fix_min_r);
	config.fix_min_g = defaults->get("FIX_MIN_G", config.fix_min_g);
	config.fix_min_b = defaults->get("FIX_MIN_B", config.fix_min_b);
	config.fix_light = defaults->get("FIX_LIGHT", config.fix_light);
	config.fix_gamma_g = defaults->get("FIX_GAMMA_G", config.fix_gamma_g);
	config.fix_gamma_b = defaults->get("FIX_GAMMA_B", config.fix_gamma_b);
}

void C41Effect::save_defaults()
{
	defaults->update("ACTIVE", config.active);
	defaults->update("COMPUTE_MAGIC", config.compute_magic);
	defaults->update("FIX_MIN_R", config.fix_min_r);
	defaults->update("FIX_MIN_G", config.fix_min_g);
	defaults->update("FIX_MIN_B", config.fix_min_b);
	defaults->update("FIX_LIGHT", config.fix_light);
	defaults->update("FIX_GAMMA_G", config.fix_gamma_g);
	defaults->update("FIX_GAMMA_B", config.fix_gamma_b);
	defaults->save();
}

void C41Effect::save_data(KeyFrame *keyframe)
{
	FileXML output;

	output.set_shared_string(keyframe->data, MESSAGESIZE);
	output.tag.set_title("C41");
	output.tag.set_property("ACTIVE", config.active);
	output.tag.set_property("COMPUTE_MAGIC", config.compute_magic);

	output.tag.set_property("FIX_MIN_R", config.fix_min_r);
	output.tag.set_property("FIX_MIN_G", config.fix_min_g);
	output.tag.set_property("FIX_MIN_B", config.fix_min_b);
	output.tag.set_property("FIX_LIGHT", config.fix_light);
	output.tag.set_property("FIX_GAMMA_G", config.fix_gamma_g);
	output.tag.set_property("FIX_GAMMA_B", config.fix_gamma_b);

	output.append_tag();
	output.tag.set_title("/C41");
	output.append_tag();
	output.terminate_string();
}

void C41Effect::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->data, strlen(keyframe->data));
	while(!input.read_tag())
	{
		if(input.tag.title_is("C41"))
		{
			config.active = input.tag.get_property("ACTIVE", config.active);
			config.compute_magic = input.tag.get_property("COMPUTE_MAGIC", config.compute_magic);
			config.fix_min_r = input.tag.get_property("FIX_MIN_R", config.fix_min_r);
			config.fix_min_g = input.tag.get_property("FIX_MIN_G", config.fix_min_g);
			config.fix_min_b = input.tag.get_property("FIX_MIN_B", config.fix_min_b);
			config.fix_light = input.tag.get_property("FIX_LIGHT", config.fix_light);
			config.fix_gamma_g = input.tag.get_property("FIX_GAMMA_G", config.fix_gamma_g);
			config.fix_gamma_b = input.tag.get_property("FIX_GAMMA_B", config.fix_gamma_b);
		}
	}
}

#if defined(C41_FAST_POW)

float C41Effect::myLog2(float i)
{
	float x, y;
	float LogBodge = 0.346607f;
	x = *(int *)&i;
	x *= 1.0 / (1 << 23); // 1/pow(2,23);
	x = x - 127;

	y = x - floorf(x);
	y = (y - y * y) * LogBodge;
	return x + y;
}

float C41Effect::myPow2(float i)
{
	float PowBodge = 0.33971f;
	float x;
	float y = i - floorf(i);
	y = (y - y * y) * PowBodge;

	x = i + 127 - y;
	x *= (1 << 23);
	*(int*)&x = (int)x;
	return x;
}

float C41Effect::myPow(float a, float b)
{
	return myPow2(b * myLog2(a));
}

#endif

void C41Effect::process_frame(VFrame *frame)
{
	load_configuration();

	get_frame(frame);

	int frame_w = frame->get_w();
	int frame_h = frame->get_h();

	switch(frame->get_color_model())
	{
	case BC_RGB888:
	case BC_YUV888:
	case BC_RGBA_FLOAT:
	case BC_RGBA8888:
	case BC_YUVA8888:
	case BC_RGB161616:
	case BC_YUV161616:
	case BC_RGBA16161616:
	case BC_YUVA16161616:
		abort_plugin(_("Only RGB Float color model is supported"));
		return; // Unsupported

	case BC_RGB_FLOAT:
		break;
	}

	if(config.compute_magic){
		// Box blur!
		if(!tmp_frame)
			tmp_frame = new VFrame(*frame);
		if(!blurry_frame)
			blurry_frame = new VFrame(*frame);

		float** rows = (float**)frame->get_rows();
		float** tmp_rows = (float**)tmp_frame->get_rows();
		float** blurry_rows = (float**)blurry_frame->get_rows();
		for(int i = 0; i < frame_h; i++)
			for(int j = 0; j < (3*frame_w); j++)
				blurry_rows[i][j] = rows[i][j];

		int boxw = 5, boxh = 5;
		// 3 passes of Box blur should be good
		int pass, x, y, y_up, y_down, x_right, x_left;
		float component;
		for(pass = 0; pass < 10; pass++)
		{
			for(y = 0; y < frame_h; y++)
				for(x = 0; x < (3 * frame_w); x++)
					tmp_rows[y][x] = blurry_rows[y][x];
			for(y = 0; y < frame_h; y++)
			{
				y_up = (y - boxh < 0)? 0 : y - boxh;
				y_down = (y + boxh >= frame_h)? frame_h - 1 : y + boxh;
				for(x = 0; x < (3 * frame_w); x++) {
					x_left = (x -(3 * boxw) < 0)? 0 : x - (3 * boxw);
					x_right = (x + (3 * boxw) >= (3 * frame_w)) ? (3 * frame_w) - 1 : x + (3 * boxw);
					component=(tmp_rows[y_down][x_right]
							+ tmp_rows[y_up][x_left]
							+ tmp_rows[y_up][x_right]
							+ tmp_rows[y_down][x_right]) / 4;
					blurry_rows[y][x]= component;
				}
			}
		}

		// Compute magic negfix values
		float minima_r = 50., minima_g = 50., minima_b = 50.;
		float maxima_r = 0., maxima_g = 0., maxima_b = 0.;

		for(int i = 0; i < frame_h; i++)
		{
			SKIP_ROW;
			float *row = (float*)blurry_frame->get_rows()[i];
			for(int j = 0; j < frame_w; j++, row += 3) {
				SKIP_COL;

				if(row[0] < minima_r) minima_r = row[0];
				if(row[0] > maxima_r) maxima_r = row[0];

				if(row[1] < minima_g) minima_g = row[1];
				if(row[1] > maxima_g) maxima_g = row[1];

				if(row[2] < minima_b) minima_b = row[2];
				if(row[2] > maxima_b) maxima_b = row[2];
			}
		}
		values.min_r = minima_r;
		values.min_g = minima_g;
		values.min_b = minima_b;
		values.light = (minima_r / maxima_r) * 0.95;
		values.gamma_g = logf(maxima_r / minima_r) / logf(maxima_g / minima_g);
		values.gamma_b = logf(maxima_r / minima_r) / logf(maxima_b / minima_b);

		// Update GUI
		send_render_gui(&values);
	}

	// Apply the transformation
	if(config.active)
	{
		for(int i = 0; i < frame_h; i++)
		{
			float *row = (float*)frame->get_rows()[i];
			for(int j = 0; j < frame_w; j++, row += 3)
			{
				row[0] = (config.fix_min_r / row[0]) - config.fix_light;
				row[1] = C41_POW_FUNC((config.fix_min_g / row[1]), config.fix_gamma_g) - config.fix_light;
				row[2] = C41_POW_FUNC((config.fix_min_b / row[2]), config.fix_gamma_b) - config.fix_light;
			}
		}
	}
}
