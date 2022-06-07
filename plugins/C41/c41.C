// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2011 Florent Delannoy <florent at plui dot es>

#include "c41.h"
#include "clip.h"
#include "bchash.h"
#include "bctitle.h"
#include "colormodels.h"
#include "colorspaces.h"
#include "filexml.h"
#include "guidelines.h"
#include "language.h"
#include "picon_png.h"
#include "pluginvclient.h"
#include "pluginwindow.h"
#include "vframe.h"

#define BLUR_W 5
#define BLUR_H 5
#define BLUR_PASSES 5
#define BLUR_PASSES_MAX 20

REGISTER_PLUGIN

C41Config::C41Config()
{
	active = 0;
	compute_magic = 0;
	postproc = 0;
	show_box = 0;

	fix_min_r = fix_min_g = fix_min_b = 0;
	fix_light = 0;
	fix_gamma_g = fix_gamma_b = fix_coef1 = fix_coef2 = 0.;
	min_col = max_col = min_row = max_row = 0;
	frame_max_col = frame_max_row = -1;
	blur_passes = BLUR_PASSES;
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
		fix_min_r == src.fix_min_r &&
		fix_min_g == src.fix_min_g &&
		fix_min_b == src.fix_min_b &&
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

C41TextBox::C41TextBox(C41Effect *plugin, double *value, int x, int y)
 : BC_TextBox(x, y, 100, 1, *value)
{
	this->plugin = plugin;
	dblvalue = value;
	intvalue = 0;
}

C41TextBox::C41TextBox(C41Effect *plugin, int *value, int x, int y)
 : BC_TextBox(x, y, 100, 1, *value)
{
	this->plugin = plugin;
	intvalue = value;
	dblvalue = 0;
}

int C41TextBox::handle_event()
{
	if(intvalue)
		*intvalue = atoi(get_text());
	else
		*dblvalue = atof(get_text());
	plugin->send_configure_change();
	return 1;
}


C41Button::C41Button(C41Effect *plugin, C41Window *window, int x, int y)
 : BC_GenericButton(x, y, _("Apply values"))
{
	this->plugin = plugin;
	this->window = window;
}

int C41Button::handle_event()
{
	plugin->config.fix_min_r = plugin->min_r;
	plugin->config.fix_min_g = plugin->min_g;
	plugin->config.fix_min_b = plugin->min_b;
	plugin->config.fix_light = plugin->light;
	plugin->config.fix_gamma_g = plugin->gamma_g;
	plugin->config.fix_gamma_b = plugin->gamma_b;
	if(plugin->coef1 > 0)
		plugin->config.fix_coef1 = plugin->coef1;
	if(plugin->coef2 > 0)
		plugin->config.fix_coef2 = plugin->coef2;
	plugin->config.frame_max_row = plugin->frame_max_row;
	plugin->config.frame_max_col = plugin->frame_max_col;

	// Set uninitalized box values
	if(!plugin->config.max_row || !plugin->config.max_col)
	{
		plugin->config.min_row = plugin->shave_min_row;
		plugin->config.max_row = plugin->shave_max_row;
		plugin->config.min_col = plugin->shave_min_col;
		plugin->config.max_col = plugin->shave_max_col;
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
	plugin->config.min_row = plugin->shave_min_row;
	plugin->config.max_row = plugin->shave_max_row;
	plugin->config.min_col = plugin->shave_min_col;
	plugin->config.max_col = plugin->shave_max_col;
	plugin->config.frame_max_row = plugin->frame_max_row;
	plugin->config.frame_max_col = plugin->frame_max_col;

	window->update();

	plugin->send_configure_change();
	return 1;
}


C41Slider::C41Slider(C41Effect *plugin, int *output, int x, int y, int max)
 : BC_ISlider(x, y, 0, 180, 180, 0, max > 0 ? max : SLIDER_LENGTH,
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

C41Window::C41Window(C41Effect *plugin, int x, int y)
 : PluginWindow(plugin, x, y, 500, 540)
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
	add_subwindow(min_r = new BC_Title(x + 80, y, "0"));
	y += 30;

	add_subwindow(new BC_Title(x, y, _("Min G:")));
	add_subwindow(min_g = new BC_Title(x + 80, y, "0"));
	y += 30;

	add_subwindow(new BC_Title(x, y, _("Min B:")));
	add_subwindow(min_b = new BC_Title(x + 80, y, "0"));
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

	y += 30;
	add_subwindow(new BC_Title(x, y, _("Blur passes:")));
	add_subwindow(blur_passes = new C41TextBox(plugin, &plugin->config.blur_passes,
		x + 80, y));

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
	blur_passes->update(plugin->config.blur_passes);

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
	min_r->update(plugin->min_r);
	min_g->update(plugin->min_g);
	min_b->update(plugin->min_b);
	light->update(plugin->light);
	gamma_g->update(plugin->gamma_g);
	gamma_b->update(plugin->gamma_b);
	// Avoid blinking
	if(plugin->coef1 > 0 || plugin->coef2 > 0)
	{
		coef1->update(plugin->coef1);
		coef2->update(plugin->coef2);
	}
	box_col_min->update(plugin->shave_min_col);
	box_col_max->update(plugin->shave_max_col);
	box_row_min->update(plugin->shave_min_row);
	box_row_max->update(plugin->shave_max_row);
}


// C41Effect
C41Effect::C41Effect(PluginServer *server)
: PluginVClient(server)
{
	tmp_frame = 0;
	blurry_frame = 0;
	pv_min = pv_max = 0;
	pv_alloc = 0;
	min_r = min_g = min_b = 0;
	light = gamma_g = gamma_b = coef1 = coef2 = 0.;
	frame_max_col = frame_max_row = 0;
	shave_min_col = shave_max_col = shave_min_row = shave_max_row = 0;
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

void C41Effect::reset_plugin()
{
	if(tmp_frame)
	{
		delete tmp_frame;
		tmp_frame = 0;
		delete blurry_frame;
		blurry_frame = 0;
		delete [] pv_min;
		pv_min = 0;
		delete [] pv_max;
		pv_max = 0;
		pv_alloc = 0;
	}
}
PLUGIN_CLASS_METHODS

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
	config.blur_passes = defaults->get("BLUR_PASSES", config.blur_passes);
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
	defaults->update("BLUR_PASSES", config.blur_passes);
	defaults->save();
}

void C41Effect::save_data(KeyFrame *keyframe)
{
	FileXML output;

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
	output.tag.set_property("BLUR_PASSES", config.blur_passes);
	output.append_tag();
	output.tag.set_title("/C41");
	output.append_tag();
	keyframe->set_data(output.string);
}

void C41Effect::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());
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
			config.blur_passes = input.tag.get_property("BLUR_PASSES", config.blur_passes);
		}
	}
}

VFrame *C41Effect::process_tmpframe(VFrame *frame)
{
	if(load_configuration())
		update_gui();

	int frame_w = frame->get_w();
	int frame_h = frame->get_h();

	switch(frame->get_color_model())
	{
	case BC_RGBA16161616:
	case BC_AYUV16161616:
		break;
	default:
		unsupported(frame->get_color_model());
		return frame;
	}

	if(config.compute_magic)
	{
		// Box blur!
		// Convert frame to RGB for magic computing
		if(!tmp_frame)
			tmp_frame = new VFrame(0, frame_w, frame_h, BC_RGBA16161616);
		if(!blurry_frame)
			blurry_frame = new VFrame(0, frame_w, frame_h, BC_RGBA16161616);

		switch(frame->get_color_model())
		{
		case BC_RGBA16161616:
			blurry_frame->copy_from(frame);
			break;

		case BC_AYUV16161616:
			blurry_frame->transfer_from(frame);
			break;
		}
		if(config.blur_passes > 0 && config.blur_passes < BLUR_PASSES_MAX)
		{
			for(int pass = 0; pass < config.blur_passes; pass++)
			{
				VFrame *swt = blurry_frame;
				blurry_frame = tmp_frame;
				tmp_frame = swt;

				for(int y = 0; y < frame_h; y++)
				{
					uint16_t *blurry_row = (uint16_t*)blurry_frame->get_row_ptr(y);
					int y_up = (y - BLUR_H < 0) ? 0 : y - BLUR_H;
					int y_down = (y + BLUR_H >= frame_h) ?
						frame_h - 1 : y + BLUR_H;
					uint16_t *tmp_up = (uint16_t*)tmp_frame->get_row_ptr(y_up);
					uint16_t *tmp_down = (uint16_t*)tmp_frame->get_row_ptr(y_down);

					for(int x = 0; x < (4 * frame_w); x++)
					{
						if(x % 4 == 3)
							continue;
						int x_left = (x - (4 * BLUR_W) < 0) ?
							0 : x - (4 * BLUR_W);
						int x_right = (x + (4 * BLUR_W) >= (4 * frame_w)) ?
							(4 * (frame_w - 1)) : x + (4 * BLUR_W);
						int component = (tmp_down[x_right] +
							tmp_up[x_left] +
							tmp_up[x_right] +
							tmp_down[x_right]) / 4;
						blurry_row[x] = component;
					}
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
			pv_min = new int[pv_alloc];
			pv_max = new int[pv_alloc];
		}

		for(int x = 0; x < pv_alloc; x++)
		{
			pv_min[x] = 0x10000;
			pv_max[x] = 0;
		}

		for(int y = 0; y < frame_h; y++)
		{
			uint16_t *row = (uint16_t*)blurry_frame->get_row_ptr(y);
			int pv;

			for(int x = 0; x < frame_w; x++)
			{
				pv = (row[0] + row[1] + row[2]) / 3;

				if(pv_min[y] > pv)
					pv_min[y] = pv;
				if(pv_max[y] < pv)
					pv_max[y] = pv;
				row += 4;
			}
		}
		for(shave_min_row = 0; shave_min_row < frame_h; shave_min_row++)
			if(pv_max[shave_min_row] - pv_min[shave_min_row] > C41_SHAVE_TOLERANCE)
				break;
		for(shave_max_row = frame_h - 1; shave_max_row > shave_min_row; shave_max_row--)
			if(pv_max[shave_max_row] - pv_min[shave_max_row] > C41_SHAVE_TOLERANCE)
				break;
		shave_max_row++;

		for(int x = 0; x < pv_alloc; x++)
		{
			pv_min[x] = 0x10000;
			pv_max[x] = 0;
		}

		for(int y = shave_min_row; y < shave_max_row; y++)
		{
			int pv;
			uint16_t *row = (uint16_t*)blurry_frame->get_row_ptr(y);

			for(int x = 0; x < frame_w; x++)
			{
				pv = (row[0] + row[1] + row[2]) / 3;

				if(pv_min[x] > pv)
					pv_min[x] = pv;
				if(pv_max[x] < pv)
					pv_max[x] = pv;
				row += 4;
			}
		}
		for(shave_min_col = 0; shave_min_col < frame_w; shave_min_col++)
			if(pv_max[shave_min_col] - pv_min[shave_min_col] > C41_SHAVE_TOLERANCE)
				break;
		for(shave_max_col = frame_w - 1; shave_max_col > shave_min_col; shave_max_col--)
			if(pv_max[shave_max_col] - pv_min[shave_max_col] > C41_SHAVE_TOLERANCE)
				break;
		shave_max_col++;

		int mrg = C41_SHAVE_BLUR * frame_w;

		if(shave_min_col < mrg)
			shave_min_col = mrg;
		if(shave_max_col > frame_w - mrg)
			shave_max_col = frame_w - mrg;

		mrg = C41_SHAVE_BLUR * frame_h;
		if(shave_min_row < mrg)
			shave_min_row = mrg;
		if(shave_max_row > frame_h - mrg)
			shave_max_row = frame_h - mrg;

		// Compute magic negfix values
		int minima_r = 0x10000, minima_g = 0x10000, minima_b = 0x10000;
		int maxima_r = 0, maxima_g = 0, maxima_b = 0;

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

		for(int i = min_row; i < max_row; i++)
		{
			uint16_t *row = (uint16_t *)blurry_frame->get_row_ptr(i);
			row += 4 * min_col;
			for(int j = min_col; j < max_col; j++)
			{

				if(row[0] < minima_r) minima_r = row[0];
				if(row[0] > maxima_r) maxima_r = row[0];

				if(row[1] < minima_g) minima_g = row[1];
				if(row[1] > maxima_g) maxima_g = row[1];

				if(row[2] < minima_b) minima_b = row[2];
				if(row[2] > maxima_b) maxima_b = row[2];
				row += 4;
			}
		}

		min_r = minima_r;
		min_g = minima_g;
		min_b = minima_b;
		light = (double)minima_r / maxima_r;

		double rsc = (double)maxima_r / minima_r;
		double gsc = (double)maxima_g / minima_g;
		double bsc = (double)maxima_b / minima_b;

		gamma_g = log(rsc) / log(gsc);
		gamma_b = log(rsc) / log(bsc);

		frame_max_row = frame_h;
		frame_max_col = frame_w;
		coef1 = -1;
		coef2 = -1;
		update_magic();
	}

	// Apply the transformation
	if(config.active)
	{
		switch(frame->get_color_model())
		{
		case BC_RGBA16161616:
			for(int i = 0; i < frame_h; i++)
			{
				uint16_t *row = (uint16_t*)frame->get_row_ptr(i);
				for(int j = 0; j < frame_w; j++)
				{
					row[0] = pixtouint((double)config.fix_min_r /
						row[0] - config.fix_light);
					row[1] = pixtouint(pow((double)config.fix_min_g /
						row[1], config.fix_gamma_g) - config.fix_light);
					row[2] = pixtouint(pow((double)config.fix_min_b / row[2],
						config.fix_gamma_b) - config.fix_light);
					row += 4;
				}
			}
			break;
		case BC_AYUV16161616:
			for(int i = 0; i < frame_h; i++)
			{
				uint16_t *row = (uint16_t*)frame->get_row_ptr(i);
				int r, g, b;
				for(int j = 0; j < frame_w; j++)
				{
					ColorSpaces::yuv_to_rgb_16(r, g, b, row[1], row[2], row[3]);
					r = pixtouint((double)config.fix_min_r / r -
						config.fix_light);
					g = pixtouint(pow((double)config.fix_min_g /
						g, config.fix_gamma_g) - config.fix_light);
					b = pixtouint(pow((double)config.fix_min_b /
						b, config.fix_gamma_b) - config.fix_light);
					ColorSpaces::rgb_to_yuv_16(r, g, b, row[1], row[2], row[3]);
					row += 4;
				}
			}
			break;
		}

		if(config.compute_magic && !config.postproc)
		{
			int min_r, min_g, min_b, max_r, max_g, max_b;
			min_r = min_g = min_b = 0x10000;
			max_r = max_g = max_b = 0;

			switch(frame->get_color_model())
			{
			case BC_RGBA16161616:
				for(int i = min_row; i < max_row; i++)
				{
					uint16_t *row = (uint16_t*)frame->get_row_ptr(i);
					row += 4 * min_col;
					for(int j = min_col; j < max_col; j++)
					{

						if(row[0] < min_r) min_r = row[0];
						if(row[0] > max_r) max_r = row[0];

						if(row[1] < min_g) min_g = row[1];
						if(row[1] > max_g) max_g = row[1];

						if(row[2] < min_b) min_b = row[2];
						if(row[2] > max_b) max_b = row[2];

						row += 4;
					}
				}
				break;
			case BC_AYUV16161616:
				for(int i = min_row; i < max_row; i++)
				{
					uint16_t *row = (uint16_t*)frame->get_row_ptr(i);
					row += 4 * min_col;
					int r, g, b;

					for(int j = min_col; j < max_col; j++)
					{
						ColorSpaces::yuv_to_rgb_16(r, g, b, row[1], row[2], row[3]);

						if(r < min_r) min_r = r;
						if(r > max_r) max_r = r;

						if(g < min_g) min_g = g;
						if(g > max_g) max_g = g;

						if(b < min_b) min_b = b;
						if(b > max_b) max_b = b;

						row += 4;
					}
				}
				break;
			}

			// Calculate postprocessing coeficents
			coef2 = min_r;
			if(min_g < coef2)
				coef2 = min_g;
			if(min_b < coef2)
				coef2 = min_b;
			coef1 = max_r;
			if(max_g > coef1)
				coef1 = max_g;
			if(max_b > coef1)
				coef1 = max_b;

			coef1 /= 0xffff;
			coef2 /= 0xffff;
			coef1 = 1.0 / (coef1 - coef2);
			coef2 = 0.065 - coef2 * coef1;
			update_magic();
		}

		GuideFrame *gf = get_guides();

		if(gf)
		{
			if(config.compute_magic && config.show_box)
			{
				gf->clear();
				if(min_row < max_row - 1)
				{
					gf->add_line(0, min_row, frame_w - 1, min_row);
					gf->add_line(0, max_row - 1, frame_w - 1, max_row - 1);
				}
				if(min_col < max_col - 1)
				{
					gf->add_line(min_col, 0, min_col, frame_h - 1);
					gf->add_line(max_col - 1, 0, max_col - 1, frame_h - 1);
				}
			}
			else
				gf->set_enabled(0);
		}
	}
	return frame;
}

uint16_t C41Effect::pixtouint(double val)
{
	int rv;

	if(config.postproc)
		val = config.fix_coef1 * val + config.fix_coef2;

	rv = round(val * 0xffff);
	return CLIP(rv, 0, 0xffff);
}

void C41Effect::update_magic()
{
	if(thread)
		thread->window->update_magic();
}
