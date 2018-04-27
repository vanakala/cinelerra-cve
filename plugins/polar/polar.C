
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

#define PLUGIN_TITLE N_("Polar")
#define PLUGIN_CLASS PolarEffect
#define PLUGIN_CONFIG_CLASS PolarConfig
#define PLUGIN_THREAD_CLASS PolarThread
#define PLUGIN_GUI_CLASS PolarWindow

#include "pluginmacros.h"

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

#include <string.h>
#include <stdint.h>


#define SQR(x) ((x) * (x))
#define WITHIN(a, b, c) ((((a) <= (b)) && ((b) <= (c))) ? 1 : 0)


class PolarEngine;


class PolarConfig
{
public:
	PolarConfig();

	void copy_from(PolarConfig &src);
	int equivalent(PolarConfig &src);
	void interpolate(PolarConfig &prev, 
		PolarConfig &next, 
		ptstime prev_pts,
		ptstime next_pts,
		ptstime current_pts);

	int polar_to_rectangular;
	float depth;
	float angle;
	int backwards;
	int invert;
	PLUGIN_CONFIG_CLASS_MEMBERS
};

class PolarDepth : public BC_FSlider
{
public:
	PolarDepth(PolarEffect *plugin, int x, int y);
	int handle_event();
	PolarEffect *plugin;
};

class PolarAngle : public BC_FSlider
{
public:
	PolarAngle(PolarEffect *plugin, int x, int y);
	int handle_event();
	PolarEffect *plugin;
};

class PolarWindow : public PluginWindow
{
public:
	PolarWindow(PolarEffect *plugin, int x, int y);

	void update();

	PolarDepth *depth;
	PolarAngle *angle;
	PLUGIN_GUI_CLASS_MEMBERS
};

PLUGIN_THREAD_HEADER

class PolarPackage : public LoadPackage
{
public:
	PolarPackage();
	int row1, row2;
};

class PolarUnit : public LoadClient
{
public:
	PolarUnit(PolarEffect *plugin, PolarEngine *server);
	void process_package(LoadPackage *package);
	PolarEffect *plugin;
};

class PolarEngine : public LoadServer
{
public:
	PolarEngine(PolarEffect *plugin, int cpus);
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
	PolarEffect *plugin;
};

class PolarEffect : public PluginVClient
{
public:
	PolarEffect(PluginServer *server);
	~PolarEffect();

	PLUGIN_CLASS_MEMBERS

	void process_realtime(VFrame *input, VFrame *output);

	void load_defaults();
	void save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);

	PolarEngine *engine;
	VFrame *temp_frame;
	VFrame *input, *output;
	int need_reconfigure;
};


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
 : PluginWindow(plugin->gui_string, 
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
		(float)1, 
		(float)100,
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
		(float)1, 
		(float)360, 
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
	need_reconfigure = 1;
	temp_frame = 0;
	engine = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

PolarEffect::~PolarEffect()
{
	if(temp_frame) delete temp_frame;
	if(engine) delete engine;
	PLUGIN_DESTRUCTOR_MACRO
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

// cause data to be stored directly in text
	output.set_shared_string(keyframe->data, MESSAGESIZE);
	output.tag.set_title("POLAR");
	output.tag.set_property("DEPTH", config.depth);
	output.tag.set_property("ANGLE", config.angle);
	output.append_tag();
	output.tag.set_title("/POLAR");
	output.append_tag();
	output.terminate_string();
}

void PolarEffect::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->data, strlen(keyframe->data));

	while(!input.read_tag())
	{
		if(input.tag.title_is("POLAR"))
		{
			config.depth = input.tag.get_property("DEPTH", config.depth);
			config.angle = input.tag.get_property("ANGLE", config.angle);
		}
	}
}

void PolarEffect::process_realtime(VFrame *input, VFrame *output)
{
	need_reconfigure |= load_configuration();

	this->input = input;
	this->output = output;

	if(EQUIV(config.depth, 0) || EQUIV(config.angle, 0))
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

		if(!engine) engine = new PolarEngine(this, PluginClient::smp + 1);

		engine->process_packages();
	}
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
		float depth,
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

#define GET_PIXEL(x, y, components, input_rows) \
	input_rows[CLIP((y), 0, ((h) - 1))] + components * CLIP((x), 0, ((w) - 1))

#define POLAR_MACRO(type, max, components, chroma_offset) \
{ \
	type **in_rows = (type**)plugin->input->get_rows(); \
	type **out_rows = (type**)plugin->output->get_rows(); \
	double values[4]; \
 \
	for(int y = pkg->row1; y < pkg->row2; y++) \
	{ \
		type *output_row = out_rows[y]; \
 \
		for(int x = 0; x < w; x++) \
		{ \
			type *output_pixel = output_row + x * components; \
			if(calc_undistorted_coords(x, \
				y, \
				w, \
				h, \
				plugin->config.depth, \
				plugin->config.angle, \
				plugin->config.polar_to_rectangular, \
				plugin->config.backwards, \
				plugin->config.invert, \
				cen_x, \
				cen_y, \
				cx, \
				cy)) \
			{ \
				type *pixel1 = GET_PIXEL((int)cx,     (int)cy,	 components, in_rows); \
				type *pixel2 = GET_PIXEL((int)cx + 1, (int)cy,	 components, in_rows); \
				type *pixel3 = GET_PIXEL((int)cx,     (int)cy + 1, components, in_rows); \
				type *pixel4 = GET_PIXEL((int)cx + 1, (int)cy + 1, components, in_rows); \
 \
				values[0] = pixel1[0]; \
				values[1] = pixel2[0]; \
				values[2] = pixel3[0]; \
				values[3] = pixel4[0]; \
				output_pixel[0] = (type)bilinear(cx, cy, values); \
 \
				values[0] = pixel1[1]; \
				values[1] = pixel2[1]; \
				values[2] = pixel3[1]; \
				values[3] = pixel4[1]; \
				output_pixel[1] = (type)bilinear(cx, cy, values); \
 \
				values[0] = pixel1[2]; \
				values[1] = pixel2[2]; \
				values[2] = pixel3[2]; \
				values[3] = pixel4[2]; \
				output_pixel[2] = (type)bilinear(cx, cy, values); \
 \
				if(components == 4) \
				{ \
					values[0] = pixel1[3]; \
					values[1] = pixel2[3]; \
					values[2] = pixel3[3]; \
					values[3] = pixel4[3]; \
					output_pixel[3] = (type)bilinear(cx, cy, values); \
				} \
			} \
			else \
			{ \
				output_pixel[0] = 0; \
				output_pixel[1] = chroma_offset; \
				output_pixel[2] = chroma_offset; \
				if(components == 4) output_pixel[3] = max; \
			} \
		} \
	} \
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

	switch(plugin->input->get_color_model())
	{
	case BC_RGB_FLOAT:
		POLAR_MACRO(float, 1, 3, 0x0)
		break;
	case BC_RGBA_FLOAT:
		POLAR_MACRO(float, 1, 4, 0x0)
		break;
	case BC_RGB888:
		POLAR_MACRO(unsigned char, 0xff, 3, 0x0)
		break;
	case BC_RGBA8888:
		POLAR_MACRO(unsigned char, 0xff, 4, 0x0)
		break;
	case BC_RGB161616:
		POLAR_MACRO(uint16_t, 0xffff, 3, 0x0)
		break;
	case BC_RGBA16161616:
		POLAR_MACRO(uint16_t, 0xffff, 4, 0x0)
		break;
	case BC_YUV888:
		POLAR_MACRO(unsigned char, 0xff, 3, 0x80)
		break;
	case BC_YUVA8888:
		POLAR_MACRO(unsigned char, 0xff, 4, 0x80)
		break;
	case BC_YUV161616:
		POLAR_MACRO(uint16_t, 0xffff, 3, 0x8000)
		break;
	case BC_YUVA16161616:
		POLAR_MACRO(uint16_t, 0xffff, 4, 0x8000)
		break;
	case BC_AYUV16161616:
		for(int y = pkg->row1; y < pkg->row2; y++)
		{
			double values[4];
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
