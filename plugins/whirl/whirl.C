// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bchash.h"
#include "bcslider.h"
#include "bctitle.h"
#include "clip.h"
#include "filexml.h"
#include "keyframe.h"
#include "vframe.h"
#include "whirl.h"

#include <stdint.h>
#include <string.h>

#define MAXRADIUS 100
#define MAXPINCH 100


PLUGIN_THREAD_METHODS

REGISTER_PLUGIN


WhirlConfig::WhirlConfig()
{
	angle = 0.0;
	pinch = 0.0;
	radius = 0.0;
}

void WhirlConfig::copy_from(WhirlConfig &src)
{
	this->angle = src.angle;
	this->pinch = src.pinch;
	this->radius = src.radius;
}

int WhirlConfig::equivalent(WhirlConfig &src)
{
	return EQUIV(this->angle, src.angle) &&
		EQUIV(this->pinch, src.pinch) &&
		EQUIV(this->radius, src.radius);
}

void WhirlConfig::interpolate(WhirlConfig &prev, 
	WhirlConfig &next, 
	ptstime prev_pts,
	ptstime next_pts,
	ptstime current_pts)
{
	PLUGIN_CONFIG_INTERPOLATE_MACRO

	this->angle = prev.angle * prev_scale + next.angle * next_scale;
	this->pinch = prev.pinch * prev_scale + next.pinch * next_scale;
	this->radius = prev.radius * prev_scale + next.radius * next_scale;
}


WhirlWindow::WhirlWindow(WhirlEffect *plugin, int x, int y)
 : PluginWindow(plugin,
	x,
	y, 
	220, 
	200)
{
	x = y = 10;

	add_subwindow(new BC_Title(x, y, _("Radius")));
	y += 20;
	add_subwindow(radius = new WhirlRadius(plugin, x, y));
	y += 40;
	add_subwindow(new BC_Title(x, y, _("Pinch")));
	y += 20;
	add_subwindow(pinch = new WhirlPinch(plugin, x, y));
	y += 40;
	add_subwindow(new BC_Title(x, y, _("Angle")));
	y += 20;
	add_subwindow(angle = new WhirlAngle(plugin, x, y));
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

void WhirlWindow::update()
{
	angle->update(plugin->config.angle);
	pinch->update(plugin->config.pinch);
	radius->update(plugin->config.radius);
}


WhirlAngle::WhirlAngle(WhirlEffect *plugin, int x, int y)
 : BC_FSlider(x, y, 0, 200, 200, 0.0, 360.0,
	plugin->config.angle)
{
	this->plugin = plugin;
}

int WhirlAngle::handle_event()
{
	plugin->config.angle = get_value();
	plugin->send_configure_change();
	return 1;
}


WhirlPinch::WhirlPinch(WhirlEffect *plugin, int x, int y)
 : BC_FSlider(x, y, 0, 200, 200, 0.0, MAXPINCH,
	plugin->config.pinch)
{
	this->plugin = plugin;
}

int WhirlPinch::handle_event()
{
	plugin->config.pinch = get_value();
	plugin->send_configure_change();
	return 1;
}


WhirlRadius::WhirlRadius(WhirlEffect *plugin, int x, int y)
 : BC_FSlider(x, y, 0, 200, 200, 0.0, MAXRADIUS, plugin->config.radius)
{
	this->plugin = plugin;
}

int WhirlRadius::handle_event()
{
	plugin->config.radius = get_value();
	plugin->send_configure_change();
	return 1;
}


WhirlEffect::WhirlEffect(PluginServer *server)
 : PluginVClient(server)
{
	engine = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

WhirlEffect::~WhirlEffect()
{
	delete engine;
	PLUGIN_DESTRUCTOR_MACRO
}

void WhirlEffect::reset_plugin()
{
	if(engine)
	{
		delete engine;
		engine = 0;
	}
}

PLUGIN_CLASS_METHODS

void WhirlEffect::load_defaults()
{
	defaults = load_defaults_file("whirl.rc");

	config.angle = defaults->get("ANGLE", config.angle);
	config.pinch = defaults->get("PINCH", config.pinch);
	config.radius = defaults->get("RADIUS", config.radius);
}

void WhirlEffect::save_defaults()
{
	defaults->update("ANGLE", config.angle);
	defaults->update("PINCH", config.pinch);
	defaults->update("RADIUS", config.radius);
	defaults->save();
}

void WhirlEffect::save_data(KeyFrame *keyframe)
{
	FileXML output;

	output.tag.set_title("WHIRL");
	output.tag.set_property("ANGLE", config.angle);
	output.tag.set_property("PINCH", config.pinch);
	output.tag.set_property("RADIUS", config.radius);
	output.append_tag();
	output.tag.set_title("/WHIRL");
	output.append_tag();
	keyframe->set_data(output.string);
}

void WhirlEffect::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());

	int result = 0;

	while(!input.read_tag())
	{
		if(input.tag.title_is("WHIRL"))
		{
			config.angle = input.tag.get_property("ANGLE", config.angle);
			config.pinch = input.tag.get_property("PINCH", config.pinch);
			config.radius = input.tag.get_property("RADIUS", config.radius);
		}
	}
}

VFrame *WhirlEffect::process_tmpframe(VFrame *input)
{
	int color_model = input->get_color_model();

	switch(color_model)
	{
	case BC_RGBA16161616:
	case BC_AYUV16161616:
		break;
	default:
		unsupported(color_model);
		return input;
	}

	if(load_configuration())
		update_gui();

	this->input = input;
	output = input;

	if(!EQUIV(config.angle, 0) &&
		(!EQUIV(config.radius, 0) || !EQUIV(config.pinch, 0)))
	{
		output = clone_vframe(input);
		output->copy_from(input);

		if(!engine)
			engine = new WhirlEngine(this, get_project_smp());
		engine->process_packages();
		release_vframe(input);
	}
	return output;
}


WhirlPackage::WhirlPackage()
 : LoadPackage()
{
}


WhirlUnit::WhirlUnit(WhirlEffect *plugin, WhirlEngine *server)
 : LoadClient(server)
{
	this->plugin = plugin;
}


static int calc_undistorted_coords(double cen_x,
			double cen_y,
			double scale_x,
			double scale_y,
			double radius,
			double radius2,
			double radius3,
			double pinch,
			double wx,
			double wy,
			double &whirl,
			double &x,
			double &y)
{
	double dx, dy;
	double d, factor;
	double dist;
	double ang, sina, cosa;
	int inside;

// Distances to center, scaled
	dx = (wx - cen_x) * scale_x;
	dy = (wy - cen_y) * scale_y;

// Distance^2 to center of *circle* (scaled ellipse)
	d = dx * dx + dy * dy;

//  If we are inside circle, then distort.
//  Else, just return the same position

	inside = (d < radius2);

	if(inside)
	{
		dist = sqrt(d / radius3) / radius;
// Pinch
		factor = pow(sin(M_PI / 2 * dist), -pinch);

		dx *= factor;
		dy *= factor;
// Whirl
		factor = 1.0 - dist;

		ang = whirl * factor * factor;

		sina = sin(ang);
		cosa = cos(ang);

		x = (cosa * dx - sina * dy) / scale_x + cen_x;
		y = (sina * dx + cosa * dy) / scale_y + cen_y;
	}
	return inside;
}

static double bilinear(double x, double y, double *values)
{
	double m0, m1;

	x = fmod(x, 1.0);
	y = fmod(y, 1.0);

	if(x < 0.0) x += 1.0;
	if(y < 0.0) y += 1.0;

	m0 = values[0] + x * (values[1] - values[0]);
	m1 = values[2] + x * (values[3] - values[2]);
	return m0 + y * (m1 - m0);
}

void WhirlUnit::process_package(LoadPackage *package)
{
	WhirlPackage *pkg = (WhirlPackage*)package;
	int w = plugin->input->get_w();
	int h = plugin->input->get_h();
	double whirl = plugin->config.angle * M_PI / 180;
	double pinch = plugin->config.pinch / MAXPINCH;
		double cx, cy;
	int ix, iy;
	double cen_x = (double)(w - 1) / 2.0;
	double cen_y = (double)(h - 1) / 2.0;
	double radius = MAX(w, h);
	double radius3 = plugin->config.radius / MAXRADIUS;
	double radius2 = radius * radius * radius3;
	double scale_x;
	double scale_y;
	double values[4];

	if(w < h)
	{
		scale_x = (double)h / w;
		scale_y = 1.0;
	}
	else
	if(w > h)
	{
		scale_x = 1.0;
		scale_y = (double)w / h;
	}
	else
	{
		scale_x = 1.0;
		scale_y = 1.0;
	}

	for(int row = pkg->row1; row <= (pkg->row2 + pkg->row1) / 2; row++)
	{
		uint16_t *top_row = (uint16_t*)plugin->output->get_row_ptr(row);
		uint16_t *bot_row = (uint16_t*)plugin->output->get_row_ptr(h - row - 1);
		uint16_t *top_p = top_row;
		uint16_t *bot_p = bot_row + 4 * w - 4;

		for(int col = 0; col < w; col++)
		{
			if(calc_undistorted_coords(cen_x, cen_y,
				scale_x, scale_y,
				radius, radius2, radius3,
				pinch, col, row, whirl,
				cx, cy))
			{
				// Inside distortion area
				// Do top
				if(cx >= 0.0)
					ix = round(cx);
				else
					ix = -(round(-cx) + 1);

				if(cy >= 0.0) \
					iy = round(cy);
				else
					iy = -(round(-cy) + 1);

				uint16_t *pixel1 = (uint16_t*)plugin->input->get_row_ptr(
						CLIP(iy, 0, (h - 1))) +
					4 * CLIP(ix, 0, (w - 1));
				uint16_t *pixel2 = (uint16_t*)plugin->input->get_row_ptr(
						CLIP(iy, 0, (h - 1))) +
					4 * CLIP(ix + 1, 0, (w - 1));
				uint16_t *pixel3 = (uint16_t*)plugin->input->get_row_ptr(
						CLIP(iy + 1, 0, (h - 1))) +
					4 * CLIP(ix, 0, (w - 1));
				uint16_t *pixel4 = (uint16_t*)plugin->input->get_row_ptr(
					CLIP(iy + 1, 0, (h - 1))) +
					4 * CLIP(ix + 1, 0, (w - 1));

				values[0] = pixel1[0];
				values[1] = pixel2[0];
				values[2] = pixel3[0];
				values[3] = pixel4[0];
				top_p[0] = round(bilinear(cx, cy, values));

				values[0] = pixel1[1];
				values[1] = pixel2[1];
				values[2] = pixel3[1];
				values[3] = pixel4[1];
				top_p[1] = round(bilinear(cx, cy, values));

				values[0] = pixel1[2];
				values[1] = pixel2[2];
				values[2] = pixel3[2];
				values[3] = pixel4[2];
				top_p[2] = round(bilinear(cx, cy, values));

				values[0] = pixel1[3];
				values[1] = pixel2[3];
				values[2] = pixel3[3];
				values[3] = pixel4[3];
				top_p[3] = round(bilinear(cx, cy, values));

				top_p += 4;

				// Do bottom
				cx = cen_x + (cen_x - cx);
				cy = cen_y + (cen_y - cy);

				if(cx >= 0.0)
					ix = round(cx);
				else
					ix = -(round(-cx) + 1);

				if(cy >= 0.0)
					iy = round(cy);
				else
					iy = -(round(-cy) + 1);

				pixel1 = (uint16_t*)plugin->input->get_row_ptr(
						CLIP(iy, 0, (h - 1))) +
					4 * CLIP(ix, 0, (w - 1));
				pixel2 = (uint16_t*)plugin->input->get_row_ptr(
						CLIP(iy, 0, (h - 1))) +
					4 * CLIP(ix + 1, 0, (w - 1));
				pixel3 = (uint16_t*)plugin->input->get_row_ptr(
						CLIP(iy + 1, 0, (h - 1))) +
					4 * CLIP(ix, 0, (w - 1));
				pixel4 = (uint16_t*)plugin->input->get_row_ptr(
						CLIP(iy + 1, 0, (h - 1))) +
					4 * CLIP(ix + 1, 0, (w - 1));

				values[0] = pixel1[0];
				values[1] = pixel2[0];
				values[2] = pixel3[0];
				values[3] = pixel4[0];
				bot_p[0] = round(bilinear(cx, cy, values));

				values[0] = pixel1[1];
				values[1] = pixel2[1];
				values[2] = pixel3[1];
				values[3] = pixel4[1];
				bot_p[1] = round(bilinear(cx, cy, values));

				values[0] = pixel1[2];
				values[1] = pixel2[2];
				values[2] = pixel3[2];
				values[3] = pixel4[2];
				bot_p[2] = round(bilinear(cx, cy, values));

				values[0] = pixel1[3];
				values[1] = pixel2[3];
				values[2] = pixel3[3];
				values[3] = pixel4[3];
				bot_p[3] = round(bilinear(cx, cy, values));

				bot_p -= 4;
			}
			else
			{
				// Outside distortion area
				// Do top
				top_p[0] = top_row[4 * col + 0];
				top_p[1] = top_row[4 * col + 1];
				top_p[2] = top_row[4 * col + 2];
				top_p[3] = top_row[4 * col + 3];

				top_p += 4;

				// Do bottom
				int bot_offset = w * 4 - col * 4 - 4;
				bot_p[0] = bot_row[bot_offset + 0];
				bot_p[1] = bot_row[bot_offset + 1];
				bot_p[2] = bot_row[bot_offset + 2];
				bot_p[3] = bot_row[bot_offset + 3];
				bot_p -= 4;
			}
		}
	}
}


WhirlEngine::WhirlEngine(WhirlEffect *plugin, int cpus)
 : LoadServer(cpus, cpus)
{
	this->plugin = plugin;
}

void WhirlEngine::init_packages()
{
	for(int i = 0; i < LoadServer::get_total_packages(); i++)
	{
		WhirlPackage *pkg = (WhirlPackage*)get_package(i);
		pkg->row1 = plugin->input->get_h() * i / LoadServer::get_total_packages();
		pkg->row2 = plugin->input->get_h() * (i + 1) / LoadServer::get_total_packages();
	}
}

LoadClient* WhirlEngine::new_client()
{
	return new WhirlUnit(plugin, this);
}

LoadPackage* WhirlEngine::new_package()
{
	return new WhirlPackage;
}
