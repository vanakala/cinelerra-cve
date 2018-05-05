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

#define SLIDER_LENGTH 1000

#include "pluginmacros.h"

#include "clip.h"
#include "bchash.h"
#include "bcslider.h"
#include "bctoggle.h"
#include "bctitle.h"
#include "colormodels.h"
#include "filexml.h"
#include "language.h"
#include "picon_png.h"
#include "plugincolors.h"
#include "pluginvclient.h"
#include "pluginwindow.h"
#include "vframe.h"

// C41_FAST_POW increases processing speed more than 10 times
// Depending on gcc version, used optimizations and cpu C41_FAST_POW may not work
// With gcc versiion >= 4.4 it seems safe to enable C41_FAST_POW
// Test some samples after you have enabled it
#define C41_FAST_POW

#if defined(C41_FAST_POW)
#define C41_POW_FUNC myPow
#else
#define C41_POW_FUNC powf
#endif

// Shave the image in order to avoid black borders
// The min max pixel value difference must be at least 0.05
#define C41_SHAVE_TOLERANCE 0.05
// Shave some amount - blurring at the edges is not good
#define C41_SHAVE_BLUR 0.20

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
	float coef1;
	float coef2;
	int shave_min_row;
	int shave_max_row;
	int shave_min_col;
	int shave_max_col;
	int frame_max_col;
	int frame_max_row;
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
	int postproc;
	int show_box;
	float fix_min_r;
	float fix_min_g;
	float fix_min_b;
	float fix_light;
	float fix_gamma_g;
	float fix_gamma_b;
	float fix_coef1;
	float fix_coef2;
	int min_col;
	int max_col;
	int min_row;
	int max_row;
	int frame_max_row;
	int frame_max_col;
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
};

class C41BoxButton : public BC_GenericButton
{
public:
	C41BoxButton(C41Effect *plugin, C41Window *window, int x, int y);
	int handle_event();

	C41Effect *plugin;
	C41Window *window;
};

class C41Slider : public BC_ISlider
{
public:
	C41Slider(C41Effect *plugin, int *output, int x, int y, int max);

	int handle_event();

	C41Effect *plugin;
	int *output;
};

class C41Window : public PluginWindow
{
public:
	C41Window(C41Effect *plugin, int x, int y);

	void update();
	void update_magic();

	C41Enable *active;
	C41Enable *compute_magic;
	C41Enable *postproc;
	C41Enable *show_box;
	BC_Title *min_r;
	BC_Title *min_g;
	BC_Title *min_b;
	BC_Title *light;
	BC_Title *gamma_g;
	BC_Title *gamma_b;
	BC_Title *coef1;
	BC_Title *coef2;
	BC_Title *box_col_min;
	BC_Title *box_col_max;
	BC_Title *box_row_min;
	BC_Title *box_row_max;
	C41TextBox *fix_min_r;
	C41TextBox *fix_min_g;
	C41TextBox *fix_min_b;
	C41TextBox *fix_light;
	C41TextBox *fix_gamma_g;
	C41TextBox *fix_gamma_b;
	C41TextBox *fix_coef1;
	C41TextBox *fix_coef2;
	C41Button *lock;
	C41BoxButton *boxlock;
	C41Slider *min_row;
	C41Slider *max_row;
	C41Slider *min_col;
	C41Slider *max_col;

	int slider_max_col;
	int slider_max_row;

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
	float fix_exepts(float ival);
	float normalize_pixel(float ival);
	uint16_t pixtouint(float val);
#if defined(C41_FAST_POW)
	float myLog2(float i) __attribute__ ((optimize(0)));
	float myPow2(float i) __attribute__ ((optimize(0)));
	float myPow(float a, float b);
#endif
	YUV yuv;
	VFrame* tmp_frame;
	VFrame* blurry_frame;
	struct magic values;

	float *pv_min;
	float *pv_max;
	int pv_alloc;

	int shave_min_row;
	int shave_max_row;
	int shave_min_col;
	int shave_max_col;
	int min_col;
	int max_col;
	int min_row;
	int max_row;

	PLUGIN_CLASS_MEMBERS
};


REGISTER_PLUGIN


// C41Config
C41Config::C41Config()
{
	active = 0;
	compute_magic = 0;
	postproc = 0;
	show_box = 0;

	fix_min_r = fix_min_g = fix_min_b = fix_light = 0.;
	fix_gamma_g = fix_gamma_b = fix_coef1 = fix_coef2 = 0.;
	min_col = max_col = min_row = max_row = 0;
	frame_max_col = frame_max_row = -1;
}

void C41Config::copy_from(C41Config &src)
{
	active = src.active;
	compute_magic = src.compute_magic;
	postproc = src.postproc;
	show_box = src.show_box;

	fix_min_r = src.fix_min_r;
	fix_min_g = src.fix_min_g;
	fix_min_b = src.fix_min_b;
	fix_light = src.fix_light;
	fix_gamma_g = src.fix_gamma_g;
	fix_gamma_b = src.fix_gamma_b;
	fix_coef1 = src.fix_coef1;
	fix_coef2 = src.fix_coef2;
	min_row = src.min_row;
	max_row = src.max_row;
	min_col = src.min_col;
	max_col = src.max_col;
	frame_max_col = src.frame_max_col;
	frame_max_row = src.frame_max_row;
}

int C41Config::equivalent(C41Config &src)
{
	return (active == src.active &&
		compute_magic == src.compute_magic &&
		postproc == src.postproc &&
		show_box == src.show_box &&
		EQUIV(fix_min_r, src.fix_min_r) &&
		EQUIV(fix_min_g, src.fix_min_g) &&
		EQUIV(fix_min_b, src.fix_min_b) &&
		EQUIV(fix_light, src.fix_light) &&
		EQUIV(fix_gamma_g, src.fix_gamma_g) &&
		EQUIV(fix_gamma_b, src.fix_gamma_b) &&
		EQUIV(fix_coef1, src.fix_coef1) &&
		EQUIV(fix_coef2, src.fix_coef2) &&
		min_row == src.min_row &&
		max_row == src.max_row &&
		min_col == src.min_col &&
		max_col == src.max_col);
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
	postproc = prev.postproc;
	show_box = prev.show_box;

	fix_min_r = prev.fix_min_r * prev_scale + next.fix_min_r * next_scale;
	fix_min_g = prev.fix_min_g * prev_scale + next.fix_min_g * next_scale;
	fix_min_b = prev.fix_min_b * prev_scale + next.fix_min_b * next_scale;
	fix_light = prev.fix_light * prev_scale + next.fix_light * next_scale;
	fix_gamma_g = prev.fix_gamma_g * prev_scale + next.fix_gamma_g * next_scale;
	fix_gamma_b = prev.fix_gamma_b * prev_scale + next.fix_gamma_b * next_scale;
	fix_coef1 = prev.fix_coef1 * prev_scale + next.fix_coef1 * next_scale;
	fix_coef2 = prev.fix_coef2 * prev_scale + next.fix_coef2 * next_scale;
	min_row = round(prev.min_row * prev_scale + next.min_row * next_scale);
	min_col = round(prev.min_col * prev_scale + next.min_col * next_scale);
	max_row = round(prev.max_row * prev_scale + next.max_row * next_scale);
	max_col = round(prev.max_col * prev_scale + next.max_col * next_scale);
	frame_max_row = prev.frame_max_row;
	frame_max_col = prev.frame_max_col;
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
 : BC_GenericButton(x, y, _("Apply values"))
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
	if(plugin->values.coef1 > 0)
		plugin->config.fix_coef1 = plugin->values.coef1;
	if(plugin->values.coef2 > 0)
		plugin->config.fix_coef2 = plugin->values.coef2;
	plugin->config.frame_max_row = plugin->values.frame_max_row;
	plugin->config.frame_max_col = plugin->values.frame_max_col;

	// Set uninitalized box values
	if(!plugin->config.max_row || !plugin->config.max_col)
	{
		plugin->config.min_row = plugin->values.shave_min_row;
		plugin->config.max_row = plugin->values.shave_max_row;
		plugin->config.min_col = plugin->values.shave_min_col;
		plugin->config.max_col = plugin->values.shave_max_col;
	}

	window->update();

	plugin->send_configure_change();
	return 1;
}


C41BoxButton::C41BoxButton(C41Effect *plugin, C41Window *window, int x, int y)
 : BC_GenericButton(x, y, _("Apply box"))
{
	this->plugin = plugin;
	this->window = window;
}

int C41BoxButton::handle_event()
{
	plugin->config.min_row = plugin->values.shave_min_row;
	plugin->config.max_row = plugin->values.shave_max_row;
	plugin->config.min_col = plugin->values.shave_min_col;
	plugin->config.max_col = plugin->values.shave_max_col;
	plugin->config.frame_max_row = plugin->values.frame_max_row;
	plugin->config.frame_max_col = plugin->values.frame_max_col;

	window->update();

	plugin->send_configure_change();
	return 1;
}


C41Slider::C41Slider(C41Effect *plugin, int *output, int x, int y, int max)
 : BC_ISlider(x,
	y,
	0,
	180,
	180,
	0,
	max > 0 ? max : SLIDER_LENGTH,
	*output)
{
	this->plugin = plugin;
	this->output = output;
}

int C41Slider::handle_event()
{
	*output = get_value();
	plugin->send_configure_change();
	return 1;
}

PLUGIN_THREAD_METHODS

// C41Window
C41Window::C41Window(C41Effect *plugin, int x, int y)
 : PluginWindow(plugin->gui_string, x, y, 500, 510)
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

	add_subwindow(new BC_Title(x, y, _("Coef 1:")));
	add_subwindow(coef1 = new BC_Title(x + 80, y, "0.0000"));
	y += 30;

	add_subwindow(new BC_Title(x, y, _("Coef 2:")));
	add_subwindow(coef2 = new BC_Title(x + 80, y, "0.0000"));
	y += 30;

	add_subwindow(lock = new C41Button(plugin, this, x, y));
	y += 30;

#define BOX_COL 120
	add_subwindow(new BC_Title(x, y, _("Box col:")));
	add_subwindow(box_col_min = new BC_Title(x + 80, y, 0));
	add_subwindow(box_col_max = new BC_Title(x + BOX_COL, y, 0));
	y += 30;

	add_subwindow(new BC_Title(x, y, _("Box row:")));
	add_subwindow(box_row_min = new BC_Title(x + 80, y, 0));
	add_subwindow(box_row_max = new BC_Title(x + BOX_COL, y, 0));
	y += 30;

	add_subwindow(boxlock = new C41BoxButton(plugin, this, x, y));

	y = 10;
	x = 250;
	add_subwindow(show_box = new C41Enable(plugin, &plugin->config.show_box, x, y, _("Show active area")));
	y += 40;

	add_subwindow(postproc = new C41Enable(plugin, &plugin->config.postproc, x, y, _("Postprocess")));
	y += 60;

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

	add_subwindow(new BC_Title(x, y, _("Coef 1:")));
	add_subwindow(fix_coef1 = new C41TextBox(plugin, &plugin->config.fix_coef1, x + 80, y));
	y += 30;

	add_subwindow(new BC_Title(x, y, _("Coef 2:")));
	add_subwindow(fix_coef2 = new C41TextBox(plugin, &plugin->config.fix_coef2, x + 80, y));
	y += 30;

	x += 60;
	add_subwindow(new BC_Title(x - 60, y, _("Col:")));
	add_subwindow(min_col = new C41Slider(plugin, &plugin->config.min_col, x, y,
		plugin->config.frame_max_col));
	y += 25;

	add_subwindow(max_col = new C41Slider(plugin, &plugin->config.max_col, x, y,
		plugin->config.frame_max_col));
	y += 25;

	add_subwindow(new BC_Title(x - 60, y, _("Row:")));
	add_subwindow(min_row = new C41Slider(plugin, &plugin->config.min_row, x, y,
		plugin->config.frame_max_row));
	y += 25;

	add_subwindow(max_row = new C41Slider(plugin, &plugin->config.max_row, x, y,
		plugin->config.frame_max_row));
	y += 25;

	slider_max_row = SLIDER_LENGTH;
	slider_max_col = SLIDER_LENGTH;

	PLUGIN_GUI_CONSTRUCTOR_MACRO
	update_magic();
}

void C41Window::update()
{
	// Updating the GUI itself
	active->update(plugin->config.active);
	compute_magic->update(plugin->config.compute_magic);
	postproc->update(plugin->config.postproc);
	show_box->update(plugin->config.show_box);

	fix_min_r->update(plugin->config.fix_min_r);
	fix_min_g->update(plugin->config.fix_min_g);
	fix_min_b->update(plugin->config.fix_min_b);
	fix_light->update(plugin->config.fix_light);
	fix_gamma_g->update(plugin->config.fix_gamma_g);
	fix_gamma_b->update(plugin->config.fix_gamma_b);
	fix_coef1->update(plugin->config.fix_coef1);
	fix_coef2->update(plugin->config.fix_coef2);

	if(plugin->config.frame_max_col > 0 && plugin->config.frame_max_row > 0 &&
			(plugin->config.frame_max_col != slider_max_col ||
			plugin->config.frame_max_row != slider_max_row))
	{
		min_row->update(200, plugin->config.min_row,
			0, plugin->config.frame_max_row);
		max_row->update(200, plugin->config.max_row,
			0, plugin->config.frame_max_row);
		min_col->update(200, plugin->config.min_col,
			0, plugin->config.frame_max_col);
		max_col->update(200, plugin->config.max_col,
			0, plugin->config.frame_max_col);
		slider_max_row = plugin->config.frame_max_row;
		slider_max_col = plugin->config.frame_max_col;
	}
	else
	{
		min_row->update(plugin->config.min_row);
		max_row->update(plugin->config.max_row);
		min_col->update(plugin->config.min_col);
		max_col->update(plugin->config.max_col);
	}
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
	// Avoid blinking
	if(plugin->values.coef1 > 0 || plugin->values.coef2 > 0)
	{
		coef1->update(plugin->values.coef1);
		coef2->update(plugin->values.coef2);
	}
	box_col_min->update(plugin->values.shave_min_col);
	box_col_max->update(plugin->values.shave_max_col);
	box_row_min->update(plugin->values.shave_min_row);
	box_row_max->update(plugin->values.shave_max_row);
}


// C41Effect
C41Effect::C41Effect(PluginServer *server)
: PluginVClient(server)
{
	tmp_frame = 0;
	blurry_frame = 0;
	pv_min = pv_max = 0;
	pv_alloc = 0;
	memset(&values, 0, sizeof(values));
	PLUGIN_CONSTRUCTOR_MACRO
}

C41Effect::~C41Effect()
{
	delete tmp_frame;
	delete blurry_frame;
	delete [] pv_min;
	delete [] pv_max;
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
	config.postproc = defaults->get("POSTPROC", config.postproc);
	config.show_box = defaults->get("SHOW_BOX", config.show_box);
	config.fix_min_r = defaults->get("FIX_MIN_R", config.fix_min_r);
	config.fix_min_g = defaults->get("FIX_MIN_G", config.fix_min_g);
	config.fix_min_b = defaults->get("FIX_MIN_B", config.fix_min_b);
	config.fix_light = defaults->get("FIX_LIGHT", config.fix_light);
	config.fix_gamma_g = defaults->get("FIX_GAMMA_G", config.fix_gamma_g);
	config.fix_gamma_b = defaults->get("FIX_GAMMA_B", config.fix_gamma_b);
	config.fix_coef1 = defaults->get("FIX_COEF1", config.fix_coef1);
	config.fix_coef2 = defaults->get("FIX_COEF2", config.fix_coef2);
	config.min_row = defaults->get("MIN_ROW", config.min_row);
	config.max_row = defaults->get("MAX_ROW", config.max_row);
	config.min_col = defaults->get("MIN_COL", config.min_col);
	config.max_col = defaults->get("MAX_COL", config.max_col);
	config.frame_max_col = defaults->get("FRAME_COL", config.frame_max_col);
	config.frame_max_row = defaults->get("FRAME_ROW", config.frame_max_row);
}

void C41Effect::save_defaults()
{
	defaults->update("ACTIVE", config.active);
	defaults->update("COMPUTE_MAGIC", config.compute_magic);
	defaults->update("POSTPROC", config.postproc);
	defaults->update("SHOW_BOX", config.show_box);
	defaults->update("FIX_MIN_R", config.fix_min_r);
	defaults->update("FIX_MIN_G", config.fix_min_g);
	defaults->update("FIX_MIN_B", config.fix_min_b);
	defaults->update("FIX_LIGHT", config.fix_light);
	defaults->update("FIX_GAMMA_G", config.fix_gamma_g);
	defaults->update("FIX_GAMMA_B", config.fix_gamma_b);
	defaults->update("FIX_COEF1", config.fix_coef1);
	defaults->update("FIX_COEF2", config.fix_coef2);
	defaults->update("MIN_ROW", config.min_row);
	defaults->update("MAX_ROW", config.max_row);
	defaults->update("MIN_COL", config.min_col);
	defaults->update("MAX_COL", config.max_col);
	defaults->update("FRAME_COL", config.frame_max_col);
	defaults->update("FRAME_ROW", config.frame_max_row);
	defaults->save();
}

void C41Effect::save_data(KeyFrame *keyframe)
{
	FileXML output;

	output.set_shared_string(keyframe->data, MESSAGESIZE);
	output.tag.set_title("C41");
	output.tag.set_property("ACTIVE", config.active);
	output.tag.set_property("COMPUTE_MAGIC", config.compute_magic);
	output.tag.set_property("POSTPROC", config.postproc);
	output.tag.set_property("SHOW_BOX", config.show_box);

	output.tag.set_property("FIX_MIN_R", config.fix_min_r);
	output.tag.set_property("FIX_MIN_G", config.fix_min_g);
	output.tag.set_property("FIX_MIN_B", config.fix_min_b);
	output.tag.set_property("FIX_LIGHT", config.fix_light);
	output.tag.set_property("FIX_GAMMA_G", config.fix_gamma_g);
	output.tag.set_property("FIX_GAMMA_B", config.fix_gamma_b);
	output.tag.set_property("FIX_COEF1", config.fix_coef1);
	output.tag.set_property("FIX_COEF2", config.fix_coef2);
	output.tag.set_property("MIN_ROW", config.min_row);
	output.tag.set_property("MAX_ROW", config.max_row);
	output.tag.set_property("MIN_COL", config.min_col);
	output.tag.set_property("MAX_COL", config.max_col);
	output.tag.set_property("FRAME_COL", config.frame_max_col);
	output.tag.set_property("FRAME_ROW", config.frame_max_row);
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
			config.postproc = input.tag.get_property("POSTPROC", config.postproc);
			config.show_box = input.tag.get_property("SHOW_BOX", config.show_box);
			config.fix_min_r = input.tag.get_property("FIX_MIN_R", config.fix_min_r);
			config.fix_min_g = input.tag.get_property("FIX_MIN_G", config.fix_min_g);
			config.fix_min_b = input.tag.get_property("FIX_MIN_B", config.fix_min_b);
			config.fix_light = input.tag.get_property("FIX_LIGHT", config.fix_light);
			config.fix_gamma_g = input.tag.get_property("FIX_GAMMA_G", config.fix_gamma_g);
			config.fix_gamma_b = input.tag.get_property("FIX_GAMMA_B", config.fix_gamma_b);
			config.fix_coef1 = input.tag.get_property("FIX_COEF1", config.fix_coef2);
			config.fix_coef2 = input.tag.get_property("FIX_COEF2", config.fix_coef2);
			config.min_row = input.tag.get_property("MIN_ROW", config.min_row);
			config.max_row = input.tag.get_property("MAX_ROW", config.max_row);
			config.min_col = input.tag.get_property("MIN_COL", config.min_col);
			config.max_col = input.tag.get_property("MAX_COL", config.max_col);
			config.frame_max_col = input.tag.get_property("FRAME_COL", config.frame_max_col);
			config.frame_max_row = input.tag.get_property("FRAME_ROW", config.frame_max_row);
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
	int pixlen;

	load_configuration();

	get_frame(frame);

	int frame_w = frame->get_w();
	int frame_h = frame->get_h();

	switch(frame->get_color_model())
	{
	case BC_RGB_FLOAT:
		pixlen = 3;
		break;
	case BC_RGBA_FLOAT:
		pixlen = 4;
		break;
	case BC_RGBA16161616:
	case BC_AYUV16161616:
		pixlen = 4;
		break;
	default:
		abort_plugin(_("Color model '%s' is not supported"),
			ColorModels::name(frame->get_color_model()));
		return; // Unsupported
	}

	if(config.compute_magic)
	{
		// Box blur!
		int i, j, k;
		float *tmp_row, *blurry_row;

		// Convert frame to RGB for magic computing
		if(!tmp_frame)
			tmp_frame = new VFrame(0, frame_w, frame_h, BC_RGB_FLOAT);
		if(!blurry_frame)
			blurry_frame = new VFrame(0, frame_w, frame_h, BC_RGB_FLOAT);

		switch(frame->get_color_model())
		{
		case BC_RGB_FLOAT:
		case BC_RGBA_FLOAT:
			for(i = 0; i < frame_h; i++)
			{
				float *row = (float*)frame->get_row_ptr(i);
				blurry_row = (float*)blurry_frame->get_row_ptr(i);
				for(j = k = 0; j < (pixlen * frame_w); j++)
				{
					if(pixlen == 4 && (j & 3) == 3)
						continue;
					blurry_row[k++] = fix_exepts(row[j]);
				}
			}
			break;

		case BC_RGBA16161616:
			for(i = 0; i < frame_h; i++)
			{
				uint16_t *row = (uint16_t*)frame->get_row_ptr(i);
				blurry_row = (float*)blurry_frame->get_row_ptr(i);
				for(j = k = 0; j < (pixlen * frame_w); j++)
				{
					if((j & 3) == 3)
						continue;
					blurry_row[k++] = (float)row[j] / 0xffff;
				}
			}
			break;

		case BC_AYUV16161616:
			for(i = 0; i < frame_h; i++)
			{
				int r, g, b;

				uint16_t *row = (uint16_t*)frame->get_row_ptr(i);
				blurry_row = (float*)blurry_frame->get_row_ptr(i);
				for(j = k = 0; j < frame_w; j++)
				{
					yuv.yuv_to_rgb_16(r, g, b, row[1], row[2], row[3]);
					blurry_row[k++] = (float)r / 0xffff;
					blurry_row[k++] = (float)g / 0xffff;
					blurry_row[k++] = (float)b / 0xffff;
					row += 4;
				}
			}
			break;
		}
		// 10 passes of Box blur should be good
		int boxw = 5, boxh = 5;
		int pass, x, y, y_up, y_down, x_right, x_left;
		float component;

		for(pass = 0; pass < 10; pass++)
		{
			tmp_frame->copy_from(blurry_frame);
			for(y = 0; y < frame_h; y++)
			{
				blurry_row = (float*)blurry_frame->get_row_ptr(y);
				y_up = (y - boxh < 0)? 0 : y - boxh;
				y_down = (y + boxh >= frame_h)? frame_h - 1 : y + boxh;
				float *tmp_up = (float*)tmp_frame->get_row_ptr(y_up);
				float *tmp_down = (float*)tmp_frame->get_row_ptr(y_down);
				for(x = 0; x < (3 * frame_w); x++)
				{
					x_left = (x - (3 * boxw) < 0)? 0 : x - (3 * boxw);
					x_right = (x + (3 * boxw) >= (3 * frame_w)) ? (3 * frame_w) - 1 : x + (3 * boxw);
					component = (tmp_down[x_right]
							+ tmp_up[x_left]
							+ tmp_up[x_right]
							+ tmp_down[x_right]) / 4;
					blurry_row[x] = component;
				}
			}
		}
		// Shave image: cut off border areas where min max difference
		// is less than C41_SHAVE_TOLERANCE
		shave_min_row = shave_min_col = 0;
		shave_max_col = frame_w;
		shave_max_row = frame_h;

		if(!pv_alloc)
		{
			pv_alloc = MAX(frame_h, frame_w);
			pv_min = new float[pv_alloc];
			pv_max = new float[pv_alloc];
		}

		for(x = 0; x < pv_alloc; x++)
		{
			pv_min[x] = 1.;
			pv_max[x] = 0.;
		}

		for(y = 0; y < frame_h; y++)
		{
			float *row = (float *)blurry_frame->get_row_ptr(y);
			float pv;
			for(x = 0; x < frame_w; x++, row += 3)
			{
				pv = (row[0] + row[1] + row[2]) / 3;

				if(pv_min[y] > pv)
					pv_min[y] = pv;
				if(pv_max[y] < pv)
					pv_max[y] = pv;
			}
		}
		for(shave_min_row = 0; shave_min_row < frame_h; shave_min_row++)
			if(pv_max[shave_min_row] - pv_min[shave_min_row] > C41_SHAVE_TOLERANCE)
				break;
		for(shave_max_row = frame_h - 1; shave_max_row > shave_min_row; shave_max_row--)
			if(pv_max[shave_max_row] - pv_min[shave_max_row] > C41_SHAVE_TOLERANCE)
				break;
		shave_max_row++;

		for(x = 0; x < pv_alloc; x++)
		{
			pv_min[x] = 1.;
			pv_max[x] = 0.;
		}

		for(y = shave_min_row; y < shave_max_row; y++)
		{
			float pv;
			float *row = (float*)blurry_frame->get_row_ptr(y);

			for(x = 0; x < frame_w; x++, row += 3)
			{
				pv = (row[0] + row[1] + row[2]) / 3;

				if(pv_min[x] > pv)
					pv_min[x] = pv;
				if(pv_max[x] < pv)
					pv_max[x] = pv;
			}
		}
		for(shave_min_col = 0; shave_min_col < frame_w; shave_min_col++)
			if(pv_max[shave_min_col] - pv_min[shave_min_col] > C41_SHAVE_TOLERANCE)
				break;
		for(shave_max_col = frame_w - 1; shave_max_col > shave_min_col; shave_max_col--)
			if(pv_max[shave_max_col] - pv_min[shave_max_col] > C41_SHAVE_TOLERANCE)
				break;
		shave_max_col++;

		// Compute magic negfix values
		float minima_r = 50., minima_g = 50., minima_b = 50.;
		float maxima_r = 0., maxima_g = 0., maxima_b = 0.;

		// Check if config_parameters are usable
		if(config.frame_max_row == frame_h && config.frame_max_col == frame_w
			&& config.min_col < config.max_col
			&& config.min_row < config.max_row
			&& config.max_col <= config.frame_max_col
			&& config.max_row <= config.frame_max_row)
		{
			min_col = config.min_col;
			max_col = config.max_col;
			min_row = config.min_row;
			max_row = config.max_row;
		}
		else
		{
			min_col = shave_min_col;
			max_col = shave_max_col;
			min_row = shave_min_row;
			max_row = shave_max_row;
		}

		int mrg = C41_SHAVE_BLUR * frame_w;
		if(min_col < mrg)
			min_col = mrg;
		if(max_col > frame_w - mrg)
			max_col = frame_w - mrg;
		mrg = C41_SHAVE_BLUR * frame_w;
		if(min_row < mrg)
			min_row = mrg;
		if(max_row > frame_h - mrg)
			max_row = frame_h - mrg;

		for(int i = min_row; i < max_row; i++)
		{
			float *row = (float*)blurry_frame->get_row_ptr(i);
			row += 3 * min_col;
			for(int j = min_col; j < max_col; j++, row += 3)
			{

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
		values.light = (minima_r / maxima_r);
		values.gamma_g = logf(maxima_r / minima_r) / logf(maxima_g / minima_g);
		values.gamma_b = logf(maxima_r / minima_r) / logf(maxima_b / minima_b);
		values.shave_min_col = shave_min_col;
		values.shave_max_col = shave_max_col;
		values.shave_min_row = shave_min_row;
		values.shave_max_row = shave_max_row;
		values.frame_max_row = frame_h;
		values.frame_max_col = frame_w;
		values.coef1 = -1;
		values.coef2 = -1;

		// Update GUI
		send_render_gui(&values);
	}

	// Apply the transformation
	if(config.active)
	{
		switch(frame->get_color_model())
		{
		case BC_RGB_FLOAT:
		case BC_RGBA_FLOAT:
			for(int i = 0; i < frame_h; i++)
			{
				float *row = (float*)frame->get_row_ptr(i);
				for(int j = 0; j < frame_w; j++, row += pixlen)
				{
					row[0] = normalize_pixel((config.fix_min_r / row[0]) - config.fix_light);
					row[1] = normalize_pixel(C41_POW_FUNC((config.fix_min_g / row[1]), config.fix_gamma_g) - config.fix_light);
					row[2] = normalize_pixel(C41_POW_FUNC((config.fix_min_b / row[2]), config.fix_gamma_b) - config.fix_light);
				}
			}
			break;
		case BC_RGBA16161616:
			for(int i = 0; i < frame_h; i++)
			{
				uint16_t *row = (uint16_t*)frame->get_row_ptr(i);
				for(int j = 0; j < frame_w; j++, row += pixlen)
				{
					row[0] = pixtouint((config.fix_min_r / ((float)row[0] / 0xffff)) - config.fix_light);
					row[1] = pixtouint(C41_POW_FUNC((config.fix_min_g / ((float)row[1] / 0xffff)), config.fix_gamma_g) - config.fix_light);
					row[2] = pixtouint(C41_POW_FUNC((config.fix_min_b / ((float)row[2] / 0xffff)), config.fix_gamma_b) - config.fix_light);
				}
			}
			break;
		case BC_AYUV16161616:
			for(int i = 0; i < frame_h; i++)
			{
				uint16_t *row = (uint16_t*)frame->get_row_ptr(i);
				int r, g, b;
				for(int j = 0; j < frame_w; j++, row += pixlen)
				{
					yuv.yuv_to_rgb_16(r, g, b, row[1], row[2], row[3]);
					r = pixtouint((config.fix_min_r / ((float)r / 0xffff)) - config.fix_light);
					g = pixtouint(C41_POW_FUNC((config.fix_min_g / ((float)g / 0xffff)), config.fix_gamma_g) - config.fix_light);
					b = pixtouint(C41_POW_FUNC((config.fix_min_b / ((float)b / 0xffff)), config.fix_gamma_b) - config.fix_light);
					yuv.rgb_to_yuv_16(r, g, b, row[1], row[2], row[3]);
				}
			}
			break;
		}
		if(config.compute_magic && !config.postproc)
		{
			float minima_r = 50., minima_g = 50., minima_b = 50.;
			float maxima_r = 0., maxima_g = 0., maxima_b = 0.;
			int min_r, min_g, min_b, max_r, max_g, max_b;
			min_r = min_g = min_b = 0x10000;
			max_r = max_g = max_b = 0;
			int cmodel = frame->get_color_model();

			switch(frame->get_color_model())
			{
			case BC_RGB_FLOAT:
			case BC_RGBA_FLOAT:
				for(int i = min_row; i < max_row; i++)
				{
					float *row = (float*)frame->get_row_ptr(i);
					row += pixlen * shave_min_col;
					for(int j = min_col; j < max_col; j++, row += pixlen)
					{
						if(row[0] < minima_r) minima_r = row[0];
						if(row[0] > maxima_r) maxima_r = row[0];

						if(row[1] < minima_g) minima_g = row[1];
						if(row[1] > maxima_g) maxima_g = row[1];

						if(row[2] < minima_b) minima_b = row[2];
						if(row[2] > maxima_b) maxima_b = row[2];
					}
				}
				break;
			case BC_RGBA16161616:
				for(int i = min_row; i < max_row; i++)
				{
					uint16_t *row = (uint16_t*)frame->get_row_ptr(i);
					row += pixlen * shave_min_col;
					for(int j = min_col; j < max_col; j++, row += pixlen)
					{

						if(row[1] < min_g) min_g = row[1];
						if(row[1] > max_g) max_g = row[1];

						if(row[2] < min_g) min_g = row[2];
						if(row[2] > max_g) max_g = row[2];
					}
				}
				break;
			case BC_AYUV16161616:
				for(int i = min_row; i < max_row; i++)
				{
					uint16_t *row = (uint16_t*)frame->get_row_ptr(i);
					row += pixlen * shave_min_col;
					int r, g, b;

					for(int j = min_col; j < max_col; j++, row += pixlen)
					{
						yuv.yuv_to_rgb_16(r, g, b, row[1], row[2], row[3]);

						if(r < min_r) min_r = r;
						if(r > max_r) max_r = r;

						if(g < min_g) min_g = g;
						if(g > max_g) max_g = g;

						if(b < min_b) min_b = b;
						if(b > max_b) max_b = b;
					}
				}
				break;
			}

			if(cmodel == BC_AYUV16161616 || cmodel == BC_RGBA16161616)
			{
				maxima_r = (float)max_r / 0xffff;
				minima_r = (float)min_r / 0xffff;
				maxima_g = (float)max_g / 0xffff;
				minima_g = (float)min_g / 0xffff;
				maxima_b = (float)max_b / 0xffff;
				minima_b = (float)min_b / 0xffff;
			}

			// Calculate postprocessing coeficents
			values.coef2 = minima_r;
			if(minima_g < values.coef2)
				values.coef2 = minima_g;
			if(minima_b < values.coef2)
				values.coef2 = minima_b;
			values.coef1 = maxima_r;
			if(maxima_g > values.coef1)
				values.coef1 = maxima_g;
			if(maxima_b > values.coef1)
				values.coef1 = maxima_b;
			// Try not to overflow RGB601
			// (235 - 16) / 256 * 0.9
			values.coef1 = 0.770 / (values.coef1 - values.coef2);
			values.coef2 = 0.065 - values.coef2 * values.coef1;
			send_render_gui(&values);
		}

		if(config.compute_magic && config.show_box)
		{
			if(min_row < max_row - 1)
			{
				switch(frame->get_color_model())
				{
				case BC_RGB_FLOAT:
				case BC_RGBA_FLOAT:
					{
						float *row1 = (float *)frame->get_row_ptr(min_row);
						float *row2 = (float *)frame->get_row_ptr(max_row - 1);

						for(int i = 0; i < frame_w; i++)
						{
							for(int j = 0; j < 3; j++)
							{
								row1[j] = 1 - row1[j];
								row2[j] = 1 - row2[j];
							}
							row1 += pixlen;
							row2 += pixlen;
						}
					}
					break;
				case BC_RGBA16161616:
					{
						uint16_t *row1 = (uint16_t*)frame->get_row_ptr(min_row);
						uint16_t *row2 = (uint16_t*)frame->get_row_ptr(max_row - 1);

						for(int i = 0; i < frame_w; i++)
						{
							for(int j = 0; j < 3; j++)
							{
								row1[j] ^= 0xffff;
								row2[j] ^= 0xffff;
							}
							row1 += pixlen;
							row2 += pixlen;
						}
					}
					break;
				case BC_AYUV16161616:
					{
						uint16_t *row1 = (uint16_t*)frame->get_row_ptr(min_row);
						uint16_t *row2 = (uint16_t*)frame->get_row_ptr(max_row - 1);

						for(int i = 0; i < frame_w; i++)
						{
							row1[1] ^= 0xffff;
							row2[1] ^= 0xffff;
							row1 += pixlen;
							row2 += pixlen;
						}
					}
					break;
				}
			}

			if(min_col < max_col - 1)
			{
				int pix1 = pixlen * min_col;
				int pix2 = pixlen * (max_col - 1);

				switch(frame->get_color_model())
				{
				case BC_RGB_FLOAT:
				case BC_RGBA_FLOAT:
					for(int i = 0; i < frame_h; i++)
					{
						float *row = (float *)frame->get_row_ptr(i);
						float *row2 = row + pix2;
						float *row1 = row + pix1;

						for(int j = 0; j < 3; j++)
						{
							row1[j] = 1 - row1[j];
							row2[j] = 1 - row2[j];
						}
					}
					break;
				case BC_RGBA16161616:
					for(int i = 0; i < frame_h; i++)
					{
						uint16_t *row = (uint16_t*)frame->get_row_ptr(i);
						uint16_t *row2 = row + pix2;
						uint16_t *row1 = row + pix1;

						for(int j = 0; j < 3; j++)
						{
							row1[j] ^= 0xffff;
							row2[j] ^= 0xffff;
						}
					}
					break;
				case BC_AYUV16161616:
					for(int i = 0; i < frame_h; i++)
					{
						uint16_t *row = (uint16_t*)frame->get_row_ptr(i);
						uint16_t *row2 = row + pix2;
						uint16_t *row1 = row + pix1;

						row1[1] ^= 0xffff;
						row2[1] ^= 0xffff;
					}
					break;
				}
			}
		}
	}
}

float C41Effect::normalize_pixel(float ival)
{
	float val = fix_exepts(ival);

	if(config.postproc)
		val = config.fix_coef1 * val + config.fix_coef2;

	CLAMP(val, 0., 1.);
	return val;
}

float C41Effect::fix_exepts(float ival)
{
	switch(fpclassify(ival))
	{
	case FP_NAN:
	case FP_SUBNORMAL:
		ival = 0;
		break;
	case FP_INFINITE:
		if(ival < 0)
			ival = 0.;
		else
			ival = 1.;
		break;
	}
	return ival;
}

uint16_t C41Effect::pixtouint(float val)
{
	if(config.postproc)
		val = config.fix_coef1 * val + config.fix_coef2;

	val = roundf(val * 0xffff);
	return CLIP(val, 0, 0xffff);
}
