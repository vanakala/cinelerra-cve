// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 1997-2011 Adam Williams <broadcast at earthling dot net>

#include "bcsignals.h"
#include "clip.h"
#include "colorspaces.h"
#include "filexml.h"
#include "color3way.h"
#include "bchash.h"
#include "picon_png.h"

#include "aggregated.h"
#include "../gamma/aggregated.h"

#include <stdio.h>
#include <string.h>


REGISTER_PLUGIN

Color3WayConfig::Color3WayConfig()
{
	for(int i = 0; i < SECTIONS; i++)
	{
		hue_x[i] = 0.0;
		hue_y[i] = 0.0;
		value[i] = 0.0;
		saturation[i] = 0.0;
	}
}

int Color3WayConfig::equivalent(Color3WayConfig &that)
{
	for(int i = 0; i < SECTIONS; i++)
	{
		if(!EQUIV(hue_x[i], that.hue_x[i]) ||
			!EQUIV(hue_y[i], that.hue_y[i]) ||
			!EQUIV(value[i], that.value[i]) ||
			!EQUIV(saturation[i], that.saturation[i])) return 0;
	}
	return 1;
}

void Color3WayConfig::copy_from(Color3WayConfig &that)
{
	for(int i = 0; i < SECTIONS; i++)
	{
		hue_x[i] = that.hue_x[i];
		hue_y[i] = that.hue_y[i];
		value[i] = that.value[i];
		saturation[i] = that.saturation[i];
	}
}

void Color3WayConfig::interpolate(Color3WayConfig &prev, Color3WayConfig &next,
	ptstime prev_pts, ptstime next_pts, ptstime current_pts)
{
	PLUGIN_CONFIG_INTERPOLATE_MACRO

	for(int i = 0; i < SECTIONS; i++)
	{
		hue_x[i] = prev.hue_x[i] * prev_scale + next.hue_x[i] * next_scale;
		hue_y[i] = prev.hue_y[i] * prev_scale + next.hue_y[i] * next_scale;
		value[i] = prev.value[i] * prev_scale + next.value[i] * next_scale;
		saturation[i] = prev.saturation[i] * prev_scale +
			next.saturation[i] * next_scale;
	}
}

void Color3WayConfig::boundaries()
{
	for(int i = 0; i < SECTIONS; i++)
	{
		double point_radius = sqrt(SQR(hue_x[i]) + SQR(hue_y[i]));

		if(point_radius > 1)
		{
			double angle = atan2(hue_x[i], hue_y[i]);
			hue_x[i] = sin(angle);
			hue_y[i] = cos(angle);
		}
	}
}

void Color3WayConfig::copy_to_all(int section)
{
	for(int i = 0; i < SECTIONS; i++)
	{
		hue_x[i] = hue_x[section];
		hue_y[i] = hue_y[section];
		value[i] = value[section];
		saturation[i] = saturation[section];
	}
}


Color3WayPackage::Color3WayPackage()
 : LoadPackage()
{
}


Color3WayUnit::Color3WayUnit(Color3WayMain *plugin, Color3WayEngine *server)
 : LoadClient(server)
{
	this->plugin = plugin;
}

// Lower = sharper curve
#define SHADOW_GAMMA 32.0
#define HIGHLIGHT_GAMMA 32.0
// Keep value == 0 from blowing up
#define FUDGE (1.0 / 4096.0)
// Scale curve from 0 - 1
#define SHADOW_BORDER (1.0 / ((1.0 / SHADOW_GAMMA + FUDGE) / FUDGE))
#define HIGHLIGHT_BORDER (1.0 / ((1.0 / HIGHLIGHT_GAMMA + FUDGE) / FUDGE))
#define SHADOW_CURVE(value) \
	(((1.0 / (((value) / SHADOW_GAMMA + FUDGE) / FUDGE)) - SHADOW_BORDER) / \
	(1.0 - SHADOW_BORDER))

#define SHADOW_LINEAR(value) (1.0 - (value))

#define HIGHLIGHT_CURVE(value) \
	(((1.0 / (((1.0 - value) / HIGHLIGHT_GAMMA + FUDGE) / FUDGE)) - HIGHLIGHT_BORDER) / \
		(1.0 - HIGHLIGHT_BORDER))

#define HIGHLIGHT_LINEAR(value) (value)

#define MIDTONE_CURVE(value, factor) \
	((factor) <= 0 ? \
	(1.0 - SHADOW_LINEAR(value) - HIGHLIGHT_CURVE(value)) : \
	(1.0 - SHADOW_CURVE(value) - HIGHLIGHT_LINEAR(value)))

#define TOTAL_TRANSFER(value, factor) \
	(factor[SHADOWS] * SHADOW_LINEAR(value) + \
	factor[MIDTONES] * MIDTONE_CURVE(value, factor[MIDTONES]) + \
	factor[HIGHLIGHTS] * HIGHLIGHT_LINEAR(value))

void Color3WayUnit::process_package(LoadPackage *package)
{
	double values[SECTIONS];
	double saturations[SECTIONS];
	double red[SECTIONS], green[SECTIONS], blue[SECTIONS];
	Color3WayPackage *pkg = (Color3WayPackage*)package;
	int w = plugin->input->get_w();

	for(int i = 0; i < SECTIONS; i++)
		plugin->calculate_rgb(&red[i], &green[i], &blue[i],
			&saturations[i], &values[i], i);

	for(int i = pkg->row1; i < pkg->row2; i++)
	{
		uint16_t *row = (uint16_t*)plugin->input->get_row_ptr(i);

		for(int j = 0; j < w; j++)
		{
			int h;
			int r, g, b;
			double s, v;
			double dr, dg, db;
			double rcoefs[SECTIONS];
			double gcoefs[SECTIONS];
			double bcoefs[SECTIONS];

			dr = row[0];
			dg = row[1];
			db = row[2];
			level_coefs(dr / plugin->color_max, rcoefs);
			level_coefs(dg / plugin->color_max, gcoefs);
			level_coefs(db / plugin->color_max, bcoefs);

			for(int k = 0; k < SECTIONS; k++)
			{
				dr += red[k] * rcoefs[k];
				dg += green[k] * gcoefs[k];
				db += blue[k] * bcoefs[k];
			}
			CLAMP(dr, 0, plugin->color_max);
			CLAMP(dg, 0, plugin->color_max);
			CLAMP(db, 0, plugin->color_max);
			r = round(dr);
			g = round(dg);
			b = round(db);
			ColorSpaces::rgb_to_hsv(r, g, b, &h, &s, &v);

			level_coefs(s, rcoefs);
			level_coefs(v, bcoefs);

			for(int k = 0; k < SECTIONS; k++)
			{
				s += saturations[k] * rcoefs[k];
				v += values[k] * bcoefs[k];
			}

			CLAMP(s, 0, 1.0);
			CLAMP(v, 0, 1.0);
			ColorSpaces::hsv_to_rgb(&r, &g, &b, h, s, v);

			row[0] = r;
			row[1] = g;
			row[2] = b;
			row += 4;
		}
	}
}

#define COEF_NOM 0.1
#define COEF_MID 0.4
#define COEF_SHFT 0.1
// low, mid, high
void Color3WayUnit::level_coefs(double value, double *coefs)
{
	double low_coef, high_coef, mid_coef;

	if(value < EPSILON)
	{
		low_coef = 1.;
		mid_coef = 0;
		high_coef = 0;
	}
	else if(value > 1 - EPSILON)
	{
		low_coef = 0;
		mid_coef = 0;
		high_coef = 1.;
	}
	else
	{
		low_coef = COEF_NOM / value - COEF_SHFT;
		high_coef = COEF_NOM / (1.0 - value) - COEF_SHFT;
		mid_coef = COEF_MID / (low_coef + high_coef);
	}

	coefs[SHADOWS] = CLIP(low_coef, 0.0, 1.0);
	coefs[MIDTONES] = CLIP(mid_coef, 0.0, 1.0);
	coefs[HIGHLIGHTS] = CLIP(high_coef, 0.0, 1.0);
}

Color3WayEngine::Color3WayEngine(Color3WayMain *plugin, int cpus)
 : LoadServer(cpus, cpus)
{
	this->plugin = plugin;
}

void Color3WayEngine::init_packages()
{
	for(int i = 0; i < LoadServer::get_total_packages(); i++)
	{
		Color3WayPackage *pkg = (Color3WayPackage*)get_package(i);

		pkg->row1 = plugin->input->get_h() * i / LoadServer::get_total_packages();
		pkg->row2 = plugin->input->get_h() * (i + 1) / LoadServer::get_total_packages();
	}
}


LoadClient* Color3WayEngine::new_client()
{
	return new Color3WayUnit(plugin, this);
}

LoadPackage* Color3WayEngine::new_package()
{
	return new Color3WayPackage;
}


Color3WayMain::Color3WayMain(PluginServer *server)
 : PluginVClient(server)
{
	engine = 0;
	for(int i = 0; i < SECTIONS; i++)
		copy_to_all[i] = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

Color3WayMain::~Color3WayMain()
{
	delete engine;
	PLUGIN_DESTRUCTOR_MACRO
}

PLUGIN_CLASS_METHODS

void Color3WayMain::process_pixel(double *r, double *g, double *b,
	double r_in, double g_in, double b_in,
	double x, double y)
{
	double r_factor[SECTIONS];
	double g_factor[SECTIONS];
	double b_factor[SECTIONS];
	double s_factor[SECTIONS];
	double v_factor[SECTIONS];
	double *factp[] = { r_factor, g_factor, b_factor };
	double color_max = ColorModels::calculate_max(input->get_color_model());
	double comp[3];

	for(int i = 0; i < SECTIONS; i++)
	{
		calculate_factors(&r_factor[i], &g_factor[i], &b_factor [i],
			x, y);
		s_factor[i] = config.saturation[i];
		v_factor[i] = config.value[i];
	}

	comp[0] = *r;
	comp[1] = *g;
	comp[2] = *b;

// Apply hue
	for(int chn = 0; chn < 3; chn++)
	{
		double value = comp[chn] / color_max;
		double *factor = factp[chn];

		value += TOTAL_TRANSFER(value, factor);
		CLAMP(value, 0, 1.0);
		comp[chn] = value;
	}
// Apply saturation/value
	double s, v;
	int h;
	int cr = round(comp[0] * color_max);
	int cg = round(comp[1] * color_max);
	int cb = round(comp[2] * color_max);

	ColorSpaces::rgb_to_hsv(cr, cg, cb, &h, &s, &v);
	v += TOTAL_TRANSFER(v, v_factor);
	s += TOTAL_TRANSFER(s, s_factor);
	s = MAX(s, 0);
	ColorSpaces::hsv_to_rgb(&cr, &cg, &cb, h, s, v);
	*r = cr / color_max;
	*g = cg / color_max;
	*b = cb / color_max;
}

void Color3WayMain::calculate_factors(double *r, double *g, double *b,
	double x, double y)
{
	int h = round(atan2(x, y) * 360.0 / 2.0 / M_PI);
	int cr, cg, cb;
	double s = sqrt(SQR(x) + SQR(y));

	if(h >= 360)
		h -= 360;
	if(h < 0)
		h += 360;

	ColorSpaces::hsv_to_rgb(&cr, &cg, &cb, h, s, 1.0);

	*r = (0xffff - cr) / 65535.0;
	*g = (0xffff - cg) / 65535.0;
	*b = (0xffff - cb) / 65535.0;
}

void Color3WayMain::calculate_rgb(double *r, double *g, double *b,
	double *sat, double *val, int section)
{
	int h = round(atan2(config.hue_x[section], config.hue_y[section]) *
		360.0 / 2.0 / M_PI);
	int cr, cg, cb;
	int cmax = color_max;
	double s = sqrt(SQR(config.hue_x[section]) + SQR(config.hue_y[section]));

	if(h >= 360)
		h -= 360;
	if(h < 0)
		h += 360;

	ColorSpaces::hsv_to_rgb(&cr, &cg, &cb, h, s, 1.0);

	*r = (cmax - cr);
	*g = (cmax - cg);
	*b = (cmax - cb);
	*sat = config.saturation[section];
	*val = config.value[section];
}

void Color3WayMain::reset_plugin()
{
	delete engine;
	engine = 0;
}

VFrame *Color3WayMain::process_tmpframe(VFrame *frame)
{
	int do_reconfigure;
	int cmodel = frame->get_color_model();

	input = frame;
	switch(cmodel)
	{
	case BC_RGBA16161616:
		break;
	default:
		unsupported(cmodel);
		return frame;
	}

	do_reconfigure = load_configuration();
	color_max = ColorModels::calculate_max(input->get_color_model());

	if(!engine)
		engine = new Color3WayEngine(this, get_project_smp());

	if(do_reconfigure)
		update_gui();

	engine->process_packages();
	return frame;
}

void Color3WayMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

	output.tag.set_title("COLOR3WAY");

	for(int i = 0; i < SECTIONS; i++)
	{
		char string[BCTEXTLEN];
		sprintf(string, "HUE_X_%d", i);
		output.tag.set_property(string, config.hue_x[i]);
		sprintf(string, "HUE_Y_%d", i);
		output.tag.set_property(string, config.hue_y[i]);
		sprintf(string, "VALUE_%d", i);
		output.tag.set_property(string, config.value[i]);
		sprintf(string, "SATURATION_%d", i);
		output.tag.set_property(string, config.saturation[i]);

		sprintf(string, "COPY_TO_ALL_%d", i);
		output.tag.set_property(string, copy_to_all[i]);
	}

	output.append_tag();
	keyframe->set_data(output.string);
}

void Color3WayMain::load_defaults()
{
	defaults = load_defaults_file("color3way.rc");

	for(int i = 0; i < SECTIONS; i++)
	{
		char string[BCTEXTLEN];

		sprintf(string, "HUE_X_%d", i);
		config.hue_x[i] = defaults->get(string, config.hue_x[i]);
		sprintf(string, "HUE_Y_%d", i);
		config.hue_y[i] = defaults->get(string, config.hue_y[i]);
		sprintf(string, "VALUE_%d", i);
		config.value[i] = defaults->get(string, config.value[i]);
		sprintf(string, "SATURATION_%d", i);
		config.saturation[i] = defaults->get(string, config.saturation[i]);

		sprintf(string, "COPY_TO_ALL_%d", i);
		copy_to_all[i] = defaults->get(string, copy_to_all[i]);
	}
}

void Color3WayMain::save_defaults()
{
	for(int i = 0; i < SECTIONS; i++)
	{
		char string[BCTEXTLEN];

		sprintf(string, "HUE_X_%d", i);
		defaults->update(string, config.hue_x[i]);
		sprintf(string, "HUE_Y_%d", i);
		defaults->update(string, config.hue_y[i]);
		sprintf(string, "VALUE_%d", i);
		defaults->update(string, config.value[i]);
		sprintf(string, "SATURATION_%d", i);
		defaults->update(string, config.saturation[i]);

		sprintf(string, "COPY_TO_ALL_%d", i);
		defaults->update(string, copy_to_all[i]);
	}
}

void Color3WayMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), strlen(keyframe->get_data()));

	while(!input.read_tag())
	{
		if(input.tag.title_is("COLOR3WAY"))
		{
			for(int i = 0; i < SECTIONS; i++)
			{
				char string[BCTEXTLEN];

				sprintf(string, "HUE_X_%d", i);
				config.hue_x[i] = input.tag.get_property(string, config.hue_x[i]);
				sprintf(string, "HUE_Y_%d", i);
				config.hue_y[i] = input.tag.get_property(string, config.hue_y[i]);
				sprintf(string, "VALUE_%d", i);
				config.value[i] = input.tag.get_property(string, config.value[i]);
				sprintf(string, "SATURATION_%d", i);
				config.saturation[i] = input.tag.get_property(string, config.saturation[i]);

				sprintf(string, "COPY_TO_ALL_%d", i);
				copy_to_all[i] = input.tag.get_property(string, copy_to_all[i]);
			}
		}
	}
}


/* FIXIT
void Color3WayMain::handle_opengl()
{
#ifdef HAVE_GL
	get_output()->to_texture();
	get_output()->enable_opengl();

	unsigned int shader = 0;
	const char *shader_stack[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	int current_shader = 0;
	int aggregate_gamma = 0;

	get_aggregation(&aggregate_gamma);

	if(aggregate_gamma)
		GAMMA_COMPILE(shader_stack, current_shader, 0)

	COLORBALANCE_COMPILE(shader_stack, 
		current_shader, 
		aggregate_gamma)

	shader = VFrame::make_shader(0, 
		shader_stack[0], 
		shader_stack[1], 
		shader_stack[2], 
		shader_stack[3], 
		shader_stack[4], 
		shader_stack[5], 
		shader_stack[6], 
		shader_stack[7], 
		0);

	if(shader > 0)
	{
		glUseProgram(shader);
		glUniform1i(glGetUniformLocation(shader, "tex"), 0);

		if(aggregate_gamma) GAMMA_UNIFORMS(shader);

		COLORBALANCE_UNIFORMS(shader);

	}

	get_output()->init_screen();
	get_output()->bind_texture(0);
	get_output()->draw_texture();
	glUseProgram(0);
	get_output()->set_opengl_state(VFrame::SCREEN);
#endif
}
	*/






