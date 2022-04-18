// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bchash.h"
#include "bctitle.h"
#include "clip.h"
#include "filexml.h"
#include "keyframe.h"
#include "language.h"
#include "polar.h"
#include "vframe.h"

#include <string.h>
#include <stdint.h>

#define SQR(x) ((x) * (x))
#define WITHIN(a, b, c) ((((a) <= (b)) && ((b) <= (c))) ? 1 : 0)


REGISTER_PLUGIN


PolarConfig::PolarConfig()
{
	angle = 0.0;
	depth = 0.0;
	backwards = 0;
	invert = 0;
	polar_to_rectangular = 1;
}

void PolarConfig::copy_from(PolarConfig &src)
{
	this->angle = src.angle;
	this->depth = src.depth;
	this->backwards = src.backwards;
	this->invert = src.invert;
	this->polar_to_rectangular = src.polar_to_rectangular;
}

int PolarConfig::equivalent(PolarConfig &src)
{
	return EQUIV(this->angle, src.angle) && EQUIV(this->depth, src.depth);
}

void PolarConfig::interpolate(PolarConfig &prev, 
		PolarConfig &next, 
		ptstime prev_pts,
		ptstime next_pts,
		ptstime current_pts)
{
	PLUGIN_CONFIG_INTERPOLATE_MACRO

	this->depth = prev.depth * prev_scale + next.depth * next_scale;
	this->angle = prev.angle * prev_scale + next.angle * next_scale;
}


PolarWindow::PolarWindow(PolarEffect *plugin, int x, int y)
 : PluginWindow(plugin,
	x, 
	y, 
	270, 
	100)
{
	x = y = 10;

	add_subwindow(new BC_Title(x, y, _("Depth:")));
	add_subwindow(depth = new PolarDepth(plugin, x + 50, y));
	y += 40;
	add_subwindow(new BC_Title(x, y, _("Angle:")));
	add_subwindow(angle = new PolarAngle(plugin, x + 50, y));
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

void PolarWindow::update()
{
	angle->update(plugin->config.angle);
	depth->update(plugin->config.depth);
}


PLUGIN_THREAD_METHODS


PolarDepth::PolarDepth(PolarEffect *plugin, int x, int y)
 : BC_FSlider(x, 
		y,
		0,
		200,
		200, 
		1.0,
		100.0,
		plugin->config.depth)
{
	this->plugin = plugin;
}

int PolarDepth::handle_event()
{
	plugin->config.depth = get_value();
	plugin->send_configure_change();
	return 1;
}


PolarAngle::PolarAngle(PolarEffect *plugin, int x, int y)
 : BC_FSlider(x, 
		y,
		0,
		200,
		200, 
		1.0,
		360.0,
		plugin->config.angle)
{
	this->plugin = plugin;
}

int PolarAngle::handle_event()
{
	plugin->config.angle = get_value();
	plugin->send_configure_change();
	return 1;
}


PolarEffect::PolarEffect(PluginServer *server)
 : PluginVClient(server)
{
	engine = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

PolarEffect::~PolarEffect()
{
	delete engine;
	PLUGIN_DESTRUCTOR_MACRO
}

void PolarEffect::reset_plugin()
{
	if(engine)
	{
		delete engine;
		engine = 0;
	}
}

PLUGIN_CLASS_METHODS

void PolarEffect::load_defaults()
{
	defaults = load_defaults_file("polar.rc");

	config.depth = defaults->get("DEPTH", config.depth);
	config.angle = defaults->get("ANGLE", config.angle);
}

void PolarEffect::save_defaults()
{
	defaults->update("DEPTH", config.depth);
	defaults->update("ANGLE", config.angle);
	defaults->save();
}

void PolarEffect::save_data(KeyFrame *keyframe)
{
	FileXML output;

	output.tag.set_title("POLAR");
	output.tag.set_property("DEPTH", config.depth);
	output.tag.set_property("ANGLE", config.angle);
	output.append_tag();
	output.tag.set_title("/POLAR");
	output.append_tag();
	keyframe->set_data(output.string);
}

void PolarEffect::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());

	while(!input.read_tag())
	{
		if(input.tag.title_is("POLAR"))
		{
			config.depth = input.tag.get_property("DEPTH", config.depth);
			config.angle = input.tag.get_property("ANGLE", config.angle);
		}
	}
}

VFrame *PolarEffect::process_tmpframe(VFrame *input)
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

	if(EQUIV(config.depth, 0) || EQUIV(config.angle, 0))
	{
		return input;
	}
	else
	{
		output = clone_vframe(input);

		if(!engine)
			engine = new PolarEngine(this, get_project_smp());

		engine->process_packages();
	}
	release_vframe(input);
	return output;
}


PolarPackage::PolarPackage()
 : LoadPackage()
{
}


PolarUnit::PolarUnit(PolarEffect *plugin, PolarEngine *server)
 : LoadClient(server)
{
	this->plugin = plugin;
}


static int calc_undistorted_coords(int wx,
		int wy,
		int w,
		int h,
		double depth,
		double angle,
		int polar_to_rectangular,
		int backwards,
		int inverse,
		double cen_x,
		double cen_y,
		double &x,
		double &y)
{
	int inside;
	double phi, phi2;
	double xx, xm, ym, yy;
	int xdiff, ydiff;
	double r;
	double m;
	double xmax, ymax, rmax;
	double x_calc, y_calc;
	double xi, yi;
	double circle, angl, t;
	int x1, x2, y1, y2;

/* initialize */
	phi = 0.0;
	r = 0.0;

	x1 = 0;
	y1 = 0;
	x2 = w;
	y2 = h;
	xdiff = x2 - x1;
	ydiff = y2 - y1;
	xm = xdiff / 2.0;
	ym = ydiff / 2.0;
	circle = depth;
	angle = angle;
	angl = (double)angle / 180.0 * M_PI;

	if(polar_to_rectangular)
	{
		if(wx >= cen_x)
		{
			if(wy > cen_y)
			{
				phi = M_PI - 
					atan(((double)(wx - cen_x)) / 
					((double)(wy - cen_y)));
				r   = sqrt(SQR(wx - cen_x) + 
					SQR(wy - cen_y));
			}
			else
			if(wy < cen_y)
			{
				phi = atan(((double)(wx - cen_x)) / 
					((double)(cen_y - wy)));
				r   = sqrt(SQR(wx - cen_x) + 
					SQR(cen_y - wy));
			}
			else
			{
				phi = M_PI / 2;
				r   = wx - cen_x;
			}
		}
		else
		if(wx < cen_x)
		{
			if(wy < cen_y)
			{
				phi = 2 * M_PI - 
					atan(((double)(cen_x -wx)) /
						((double)(cen_y - wy)));
				r   = sqrt(SQR(cen_x - wx) + 
					SQR(cen_y - wy));
			}
			else 
			if(wy > cen_y)
			{
				phi = M_PI + 
					atan(((double)(cen_x - wx)) / 
					((double)(wy - cen_y)));
				r   = sqrt(SQR(cen_x - wx) + 
					SQR(wy - cen_y));
			}
			else
			{
				phi = 1.5 * M_PI;
				r   = cen_x - wx;
			}
		}
		if (wx != cen_x)
		{
			m = fabs(((double)(wy - cen_y)) / 
				((double)(wx - cen_x)));
		}
		else
		{
		    m = 0;
		}

		if(m <= ((double)(y2 - y1) / 
			(double)(x2 - x1)))
		{
			if(wx == cen_x)
			{
				xmax = 0;
				ymax = cen_y - y1;
			}
			else
			{
				xmax = cen_x - x1;
				ymax = m * xmax;
			}
		}
		else
		{
			ymax = cen_y - y1;
			xmax = ymax / m;
		}

		rmax = sqrt((double)(SQR(xmax) + SQR(ymax)));

		t = ((cen_y - y1) < (cen_x - x1)) ? (cen_y - y1) : (cen_x - x1);
		rmax = (rmax - t) / 100 * (100 - circle) + t;

		phi = fmod(phi + angl, 2 * M_PI);

		if(backwards)
			x_calc = x2 - 1 - (x2 - x1 - 1) / (2 * M_PI) * phi;
		else
			x_calc = (x2 - x1 - 1) / (2 * M_PI) * phi + x1;

		if(inverse)
			y_calc = (y2 - y1) / rmax * r + y1;
		else
			y_calc = y2 - (y2 - y1) / rmax * r;

		xi = (int)(x_calc + 0.5);
		yi = (int)(y_calc + 0.5);

		if(WITHIN(0, xi, w - 1) && WITHIN(0, yi, h - 1))
		{
			x = x_calc;
			y = y_calc;
			inside = 1;
		}
		else
		{
			inside = 0;
		}
	}
	else
	{
		if(backwards)
			phi = (2 * M_PI) * (x2 - wx) / xdiff;
		else
			phi = (2 * M_PI) * (wx - x1) / xdiff;

		phi = fmod (phi + angl, 2 * M_PI);

		if(phi >= 1.5 * M_PI)
			phi2 = 2 * M_PI - phi;
		else
		if (phi >= M_PI)
			phi2 = phi - M_PI;
		else
		if(phi >= 0.5 * M_PI)
			phi2 = M_PI - phi;
		else
			phi2 = phi;

		xx = tan (phi2);
		if(xx != 0)
			m = (double)1.0 / xx;
		else
			m = 0;

		if(m <= ((double)(ydiff) / (double)(xdiff)))
		{
			if(phi2 == 0)
			{
				xmax = 0;
				ymax = ym - y1;
			}
			else
			{
				xmax = xm - x1;
				ymax = m * xmax;
			}
		}
		else
		{
			ymax = ym - y1;
			xmax = ymax / m;
		}

		rmax = sqrt((double)(SQR(xmax) + SQR(ymax)));

		t = ((ym - y1) < (xm - x1)) ? (ym - y1) : (xm - x1);

		rmax = (rmax - t) / 100.0 * (100 - circle) + t;

		if(inverse)
			r = rmax * (double)((wy - y1) / (double)(ydiff));
		else
			r = rmax * (double)((y2 - wy) / (double)(ydiff));

		xx = r * sin (phi2);
		yy = r * cos (phi2);

		if(phi >= 1.5 * M_PI)
		{
			x_calc = (double)xm - xx;
			y_calc = (double)ym - yy;
		}
		else
		if(phi >= M_PI)
		{
			x_calc = (double)xm - xx;
			y_calc = (double)ym + yy;
		}
		else
		if(phi >= 0.5 * M_PI)
		{
			x_calc = (double)xm + xx;
			y_calc = (double)ym + yy;
		}
		else
		{
			x_calc = (double)xm + xx;
			y_calc = (double)ym - yy;
		}

		xi = (int)(x_calc + 0.5);
		yi = (int)(y_calc + 0.5);

		if(WITHIN(0, xi, w - 1) && 
			WITHIN(0, yi, h - 1)) 
		{
			x = x_calc;
			y = y_calc;

			inside = 1;
		}
		else
		{
			inside = 0;
		}
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

void PolarUnit::process_package(LoadPackage *package)
{
	PolarPackage *pkg = (PolarPackage*)package;
	int w = plugin->input->get_w();
	int h = plugin->input->get_h();
	double cx;
	double cy;
	double cen_x = (double)(w - 1) / 2.0;
	double cen_y = (double)(h - 1) / 2.0;
	double values[4];

	switch(plugin->input->get_color_model())
	{
	case BC_RGBA16161616:
		for(int y = pkg->row1; y < pkg->row2; y++)
		{
			uint16_t *output_row = (uint16_t*)plugin->output->get_row_ptr(y);

			for(int x = 0; x < w; x++)
			{
				uint16_t *output_pixel = output_row + x * 4;

				if(calc_undistorted_coords(x, y, w, h,
					plugin->config.depth,
					plugin->config.angle,
					plugin->config.polar_to_rectangular,
					plugin->config.backwards,
					plugin->config.invert,
					cen_x, cen_y, cx, cy))
				{
					uint16_t *pixel1 =
						(uint16_t*)plugin->input->get_row_ptr(CLIP((int)cy, 0, (h - 1))) +
							4 * CLIP((int)cx, 0, (w - 1));
					uint16_t *pixel2 = (uint16_t*)plugin->input->get_row_ptr(CLIP((int)cy, 0, (h - 1))) +
						4 * CLIP(((int)cx + 1), 0, (w - 1));
					uint16_t *pixel3 = (uint16_t*)plugin->input->get_row_ptr(CLIP(((int)cy + 1), 0, (h - 1))) +
						4 * CLIP(((int)cx), 0, (w - 1));
					uint16_t *pixel4 = (uint16_t *)plugin->input->get_row_ptr(CLIP(((int)cy + 1), 0, (h - 1))) +
						4 * CLIP(((int)cx + 1), 0, (w - 1));

					values[0] = pixel1[0];
					values[1] = pixel2[0];
					values[2] = pixel3[0];
					values[3] = pixel4[0];
					output_pixel[0] = (uint16_t)bilinear(cx, cy, values);

					values[0] = pixel1[1];
					values[1] = pixel2[1];
					values[2] = pixel3[1];
					values[3] = pixel4[1];
					output_pixel[1] = (uint16_t)bilinear(cx, cy, values);

					values[0] = pixel1[2];
					values[1] = pixel2[2];
					values[2] = pixel3[2];
					values[3] = pixel4[2];
					output_pixel[2] = (uint16_t)bilinear(cx, cy, values);

					values[0] = pixel1[3];
					values[1] = pixel2[3];
					values[2] = pixel3[3];
					values[3] = pixel4[3];
					output_pixel[3] = (uint16_t)bilinear(cx, cy, values);
				}
				else
				{
					output_pixel[0] = 0;
					output_pixel[1] = 0;
					output_pixel[2] = 0;
					output_pixel[3] = 0xffff;
				}
			}
		}
		break;

	case BC_AYUV16161616:
		for(int y = pkg->row1; y < pkg->row2; y++)
		{
			uint16_t *output_row = (uint16_t*)plugin->output->get_row_ptr(y);

			for(int x = 0; x < w; x++)
			{
				uint16_t *output_pixel = output_row + x * 4;

				if(calc_undistorted_coords(x, y, w, h,
					plugin->config.depth,
					plugin->config.angle,
					plugin->config.polar_to_rectangular,
					plugin->config.backwards,
					plugin->config.invert,
					cen_x, cen_y, cx, cy))
				{
					uint16_t *pixel1 = (uint16_t *)plugin->input->get_row_ptr((int)cy) +
						4 * CLIP((int)cx, 0, w - 1);
					uint16_t *pixel2 = (uint16_t *)plugin->input->get_row_ptr((int)cy) +
						4 * CLIP((int)cx + 1, 0, (w - 1));
					uint16_t *pixel3 = (uint16_t *)plugin->input->get_row_ptr((int)cy + 1) +
						4 * CLIP(((int)cx), 0, (w - 1));
					uint16_t *pixel4 = (uint16_t *)plugin->input->get_row_ptr((int)cy + 1) +
						4 * CLIP((int)cx + 1, 0, w - 1);

					values[0] = pixel1[0];
					values[1] = pixel2[0];
					values[2] = pixel3[0];
					values[3] = pixel4[0];
					output_pixel[0] = (uint16_t)bilinear(cx, cy, values);

					values[0] = pixel1[1];
					values[1] = pixel2[1];
					values[2] = pixel3[1];
					values[3] = pixel4[1];
					output_pixel[1] = (uint16_t)bilinear(cx, cy, values);

					values[0] = pixel1[2];
					values[1] = pixel2[2];
					values[2] = pixel3[2];
					values[3] = pixel4[2];
					output_pixel[2] = (uint16_t)bilinear(cx, cy, values);

					values[0] = pixel1[3];
					values[1] = pixel2[3];
					values[2] = pixel3[3];
					values[3] = pixel4[3];
					output_pixel[3] = (uint16_t)bilinear(cx, cy, values);
				}
				else
				{
					output_pixel[0] = 0xffff;
					output_pixel[1] = 0;
					output_pixel[2] = 0x8000;
					output_pixel[3] = 0x8000;
				}
			}
		}
		break;
	}
}


PolarEngine::PolarEngine(PolarEffect *plugin, int cpus)
 : LoadServer(cpus, cpus)
{
	this->plugin = plugin;
}

void PolarEngine::init_packages()
{
	for(int i = 0; i < LoadServer::get_total_packages(); i++)
	{
		PolarPackage *pkg = (PolarPackage*)get_package(i);
		pkg->row1 = plugin->input->get_h() * i / LoadServer::get_total_packages();
		pkg->row2 = plugin->input->get_h() * (i + 1) / LoadServer::get_total_packages();
	}
}

LoadClient* PolarEngine::new_client()
{
	return new PolarUnit(plugin, this);
}

LoadPackage* PolarEngine::new_package()
{
	return new PolarPackage;
}
