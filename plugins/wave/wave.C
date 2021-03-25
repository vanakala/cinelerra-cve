// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>


#include "bchash.h"
#include "bctitle.h"
#include "clip.h"
#include "filexml.h"
#include "keyframe.h"
#include "pluginvclient.h"
#include "pluginwindow.h"
#include "vframe.h"
#include "wave.h"

#include <math.h>
#include <stdint.h>
#include <string.h>

#define SMEAR 0
#define BLACKEN 1


WaveConfig::WaveConfig()
{
	mode = SMEAR;
	reflective = 0;
	amplitude = 10;
	phase = 0;
	wavelength = 10;
}

void WaveConfig::copy_from(WaveConfig &src)
{
	this->mode = src.mode;
	this->reflective = src.reflective;
	this->amplitude = src.amplitude;
	this->phase = src.phase;
	this->wavelength = src.wavelength;
}

int WaveConfig::equivalent(WaveConfig &src)
{
	return (this->mode == src.mode) &&
		EQUIV(this->reflective, src.reflective) &&
		EQUIV(this->amplitude, src.amplitude) &&
		EQUIV(this->phase, src.phase) &&
		EQUIV(this->wavelength, src.wavelength);
}

void WaveConfig::interpolate(WaveConfig &prev, 
	WaveConfig &next,
	ptstime prev_pts,
	ptstime next_pts,
	ptstime current_pts)
{
	PLUGIN_CONFIG_INTERPOLATE_MACRO

	this->amplitude = prev.amplitude * prev_scale + next.amplitude * next_scale;
	this->phase = prev.phase * prev_scale + next.phase * next_scale;
	this->wavelength = prev.wavelength * prev_scale + next.wavelength * next_scale;
	this->mode = prev.mode;
	this->reflective = prev.reflective;
}


WaveAmplitude::WaveAmplitude(WaveEffect *plugin, int x, int y)
 : BC_FSlider(x, y, 0, 200, 200, 0.0, 100.0,
	plugin->config.amplitude)
{
	this->plugin = plugin;
}

int WaveAmplitude::handle_event()
{
	plugin->config.amplitude = get_value();
	plugin->send_configure_change();
	return 1;
}


WavePhase::WavePhase(WaveEffect *plugin, int x, int y)
 : BC_FSlider(x, y, 0, 200, 200, 0.0, 360.0,
	plugin->config.phase)
{
	this->plugin = plugin;
}

int WavePhase::handle_event()
{
	plugin->config.phase = get_value();
	plugin->send_configure_change();
	return 1;
}


WaveLength::WaveLength(WaveEffect *plugin, int x, int y)
 : BC_FSlider(x, y, 0, 200, 200, 0.0, 50.0,
	plugin->config.wavelength)
{
	this->plugin = plugin;
}

int WaveLength::handle_event()
{
	plugin->config.wavelength = get_value();
	plugin->send_configure_change();
	return 1;
}


WaveWindow::WaveWindow(WaveEffect *plugin, int x, int y)
 : PluginWindow(plugin,
	x,
	y, 
	335,
	150)
{
	int x1 = 115;
	x = y = 10;

	add_subwindow(new BC_Title(x, y, _("Amplitude:")));
	add_subwindow(amplitude = new WaveAmplitude(plugin, x1, y));
	y += 30;
	add_subwindow(new BC_Title(x, y, _("Phase:")));
	add_subwindow(phase = new WavePhase(plugin, x1, y));
	y += 30;
	add_subwindow(new BC_Title(x, y, _("Wavelength:")));
	add_subwindow(wavelength = new WaveLength(plugin, x1, y));
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}


void WaveWindow::update()
{
	amplitude->update(plugin->config.amplitude);
	phase->update(plugin->config.phase);
	wavelength->update(plugin->config.wavelength);
}


PLUGIN_THREAD_METHODS


REGISTER_PLUGIN


WaveEffect::WaveEffect(PluginServer *server)
 : PluginVClient(server)
{
	engine = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

WaveEffect::~WaveEffect()
{
	delete engine;
	PLUGIN_DESTRUCTOR_MACRO
}

void WaveEffect::reset_plugin()
{
	if(engine)
	{
		delete engine;
		engine = 0;
	}
}

PLUGIN_CLASS_METHODS

void WaveEffect::load_defaults()
{
	defaults = load_defaults_file("wave.rc");

	config.mode = defaults->get("MODE", config.mode);
	config.reflective = defaults->get("REFLECTIVE", config.reflective);
	config.amplitude = defaults->get("AMPLITUDE", config.amplitude);
	config.phase = defaults->get("PHASE", config.phase);
	config.wavelength = defaults->get("WAVELENGTH", config.wavelength);
}

void WaveEffect::save_defaults()
{
	defaults->update("MODE", config.mode);
	defaults->update("REFLECTIVE", config.reflective);
	defaults->update("AMPLITUDE", config.amplitude);
	defaults->update("PHASE", config.phase);
	defaults->update("WAVELENGTH", config.wavelength);
	defaults->save();
}

void WaveEffect::save_data(KeyFrame *keyframe)
{
	FileXML output;

	output.tag.set_title("WAVE");
	output.tag.set_property("MODE", config.mode);
	output.tag.set_property("REFLECTIVE", config.reflective);
	output.tag.set_property("AMPLITUDE", config.amplitude);
	output.tag.set_property("PHASE", config.phase);
	output.tag.set_property("WAVELENGTH", config.wavelength);
	output.append_tag();
	output.tag.set_title("/WAVE");
	output.append_tag();
	keyframe->set_data(output.string);
}

void WaveEffect::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());

	while(!input.read_tag())
	{
		if(input.tag.title_is("WAVE"))
		{
			config.mode = input.tag.get_property("MODE", config.mode);
			config.reflective = input.tag.get_property("REFLECTIVE", config.reflective);
			config.amplitude = input.tag.get_property("AMPLITUDE", config.amplitude);
			config.phase = input.tag.get_property("PHASE", config.phase);
			config.wavelength = input.tag.get_property("WAVELENGTH", config.wavelength);
		}
	}
}

VFrame *WaveEffect::process_tmpframe(VFrame *input)
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

	if(!EQUIV(config.amplitude, 0) &&  !EQUIV(config.wavelength, 0))
	{
		output = clone_vframe(input);

		if(!engine)
			engine = new WaveServer(this, get_project_smp());

		engine->process_packages();
		release_vframe(input);
	}
	return output;
}


WavePackage::WavePackage()
 : LoadPackage()
{
}


WaveUnit::WaveUnit(WaveEffect *plugin, WaveServer *server)
 : LoadClient(server)
{
	this->plugin = plugin;
}

#define WITHIN(a, b, c) ((((a) <= (b)) && ((b) <= (c))) ? 1 : 0)

static double bilinear(double  x,
	double  y,
	double  *v)
{
	double m0, m1;
	x = fmod(x, 1.0);
	y = fmod(y, 1.0);

	if(x < 0)
		x += 1.0;
	if(y < 0)
		y += 1.0;

	m0 = (1.0 - x) * v[0] + x * v[1];
	m1 = (1.0 - x) * v[2] + x * v[3];

	return((1.0 - y) * m0 + y * m1);
}

void WaveUnit::process_package(LoadPackage *package)
{
	WavePackage *pkg = (WavePackage*)package;
	int w = plugin->input->get_w();
	int h = plugin->input->get_h();
	int row_size = w * 4;
	double cen_x, cen_y;       /* Center of wave */
	double xhsiz, yhsiz;       /* Half size of selection */
	double radius, radius2;    /* Radius and radius^2 */
	double amnt, d;
	double needx, needy;
	double dx, dy;
	double xscale, yscale;
	double wavelength;
	int xi, yi;
	double values[4];
	double val;
	int x1, y1, x2, y2;
	int x1_in, y1_in, x2_in, y2_in;
	double phase = plugin->config.phase * M_PI / 180;

	x1 = y1 = 0;
	x2 = w;
	y2 = h;
	cen_x = (x2 - 1 + x1) / 2.0;
	cen_y = (y2 - 1 + y1) / 2.0;
	xhsiz = (x2 - x1) / 2.0;
	yhsiz = (y2 - y1) / 2.0;

	if (xhsiz < yhsiz)
	{
		xscale = yhsiz / xhsiz;
		yscale = 1.0;
	}
	else if (xhsiz > yhsiz)
	{
		xscale = 1.0;
		yscale = xhsiz / yhsiz;
	}
	else
	{
		xscale = 1.0;
		yscale = 1.0;
	}

	radius  = MAX(xhsiz, yhsiz);
	radius2 = radius * radius;

	wavelength = plugin->config.wavelength / 100 * radius;

	switch(plugin->input->get_color_model())
	{
	case BC_RGBA16161616:
		for(int y = pkg->row1; y < pkg->row2; y++) \
		{
			uint16_t *dest = (uint16_t*)plugin->output->get_row_ptr(y);

			for(int x = x1; x < x2; x++)
			{
				dx = (x - cen_x) * xscale;
				dy = (y - cen_y) * yscale;
				d = sqrt(dx * dx + dy * dy);

				if(plugin->config.reflective)
				{
					amnt = plugin->config.amplitude *
						fabs(sin(((d / wavelength) *
						(2.0 * M_PI) + phase)));

					needx = (amnt * dx) / xscale + cen_x;
					needy = (amnt * dy) / yscale + cen_y;
				}
				else
				{
					amnt = plugin->config.amplitude *
						sin(((d / wavelength) *
						(2.0 * M_PI) + phase));

					needx = (amnt + dx) / xscale + cen_x;
					needy = (amnt + dy) / yscale + cen_y;
				}

				xi = round(needx);
				yi = round(needy);

				if(plugin->config.mode == SMEAR)
				{
					if(xi > w - 2)
						xi = w - 2;
					else if(xi < 0)
						xi = 0;

					if(yi > h - 2)
						yi = h - 2;
					else if(yi < 0)
						yi = 0;
				}

				uint16_t *p = (uint16_t*)plugin->input->get_row_ptr(
						CLIP(yi, 0, h - 1)) +
					CLIP(xi, 0, w - 1) * 4;
				x1_in = WITHIN(0, xi, w - 1);
				y1_in = WITHIN(0, yi, h - 1);
				x2_in = WITHIN(0, xi + 1, w - 1);
				y2_in = WITHIN(0, yi + 1, h - 1);

				for(int k = 0; k < 4; k++)
				{
					if(x1_in && y1_in)
						values[0] = *(p + k);
					else
						values[0] = 0;

					if(x2_in && y1_in)
						values[1] = *(p + 4 + k);
					else
						values[1] = 0;

					if(x1_in && y2_in)
						values[2] = *(p + row_size + k);
					else
						values[2] = 0;

					if(x2_in && y2_in)
						values[3] = *(p + row_size + 4 + k);
					else
						values[3] = 0;

					val = bilinear(needx, needy, values);
					*dest++ = val;
				}
			}
		}
		break;

	case BC_AYUV16161616:
		for(int y = pkg->row1; y < pkg->row2; y++)
		{
			uint16_t *dest = (uint16_t*)plugin->output->get_row_ptr(y);

			for(int x = x1; x < x2; x++)
			{
				dx = (x - cen_x) * xscale;
				dy = (y - cen_y) * yscale;
				d = sqrt(dx * dx + dy * dy);

				if(plugin->config.reflective) \
				{
					amnt = plugin->config.amplitude *
						fabs(sin(((d / wavelength) *
						(2.0 * M_PI) + phase)));

					needx = (amnt * dx) / xscale + cen_x;
					needy = (amnt * dy) / yscale + cen_y;
				}
				else
				{
					amnt = plugin->config.amplitude *
						sin(((d / wavelength) *
						(2.0 * M_PI) + phase));

					needx = (amnt + dx) / xscale + cen_x;
					needy = (amnt + dy) / yscale + cen_y;
				}

				xi = round(needx);
				yi = round(needy);

				if(plugin->config.mode == SMEAR)
				{
					if(xi > w - 2)
						xi = w - 2;
					else if(xi < 0)
						xi = 0;

					if(yi > h - 2)
						yi = h - 2;
					else if(yi < 0)
						yi = 0;
				}

				uint16_t *p = (uint16_t*)plugin->input->get_row_ptr(
					CLIP(yi, 0, h - 1)) + CLIP(xi, 0, w - 1) * 4;
				x1_in = WITHIN(0, xi, w - 1);
				y1_in = WITHIN(0, yi, h - 1);
				x2_in = WITHIN(0, xi + 1, w - 1);
				y2_in = WITHIN(0, yi + 1, h - 1);

				for(int k = 0; k < 4; k++)
				{
					if(x1_in && y1_in)
						values[0] = *(p + k);
					else
						values[0] = ((k == 1 || k == 2) ? 0 : 0x8000);

					if(x2_in && y1_in)
						values[1] = *(p + 4 + k);
					else
						values[1] = ((k == 1 || k == 1) ? 0 : 0x8000);

					if(x1_in && y2_in)
						values[2] = *(p + row_size + k);
					else
						values[2] = ((k == 1 || k == 2) ? 0 : 0x8000);

					if(x2_in && y2_in)
						values[3] = *(p + row_size + 4 + k);
					else
						values[3] = ((k == 1 || k == 2) ? 0 : 0x8000);

					val = bilinear(needx, needy, values);

					*dest++ = val;
				}
			}
		}
		break;
	}
}

WaveServer::WaveServer(WaveEffect *plugin, int cpus)
 : LoadServer(cpus, cpus)
{
	this->plugin = plugin;
}

void WaveServer::init_packages()
{
	for(int i = 0; i < LoadServer::get_total_packages(); i++)
	{
		WavePackage *pkg = (WavePackage*)get_package(i);
		pkg->row1 = plugin->input->get_h() * i / LoadServer::get_total_packages();
		pkg->row2 = plugin->input->get_h() * (i + 1) / LoadServer::get_total_packages();
	}
}


LoadClient* WaveServer::new_client()
{
	return new WaveUnit(plugin, this);
}

LoadPackage* WaveServer::new_package()
{
	return new WavePackage;
}

