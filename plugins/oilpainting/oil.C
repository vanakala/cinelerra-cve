// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bchash.h"
#include "bctitle.h"
#include "bctoggle.h"
#include "bcslider.h"
#include "clip.h"
#include "filexml.h"
#include "keyframe.h"
#include "loadbalance.h"
#include "oil.h"
#include "picon_png.h"
#include "pluginvclient.h"
#include "pluginwindow.h"
#include "vframe.h"

#include <math.h>
#include <stdint.h>
#include <string.h>

#define HIST_SIZE 256

// Algorithm by Torsten Martinsen
// Ported to Cinelerra by Heroine Virtual Ltd.

OilConfig::OilConfig()
{
	radius = 5;
	use_intensity = 0;
}

void OilConfig::copy_from(OilConfig &src)
{
	this->radius = src.radius;
	this->use_intensity = src.use_intensity;
}

int OilConfig::equivalent(OilConfig &src)
{
	return (EQUIV(this->radius, src.radius) &&
		this->use_intensity == src.use_intensity);
}

void OilConfig::interpolate(OilConfig &prev,
	OilConfig &next,
	ptstime prev_pts,
	ptstime next_pts,
	ptstime current_pts)
{
	PLUGIN_CONFIG_INTERPOLATE_MACRO
	this->radius = prev.radius * prev_scale + next.radius * next_scale;
	this->use_intensity = prev.use_intensity;
}


OilRadius::OilRadius(OilEffect *plugin, int x, int y)
 : BC_FSlider(x, y, 0, 200, 200,
	0.0, 30.0, plugin->config.radius)
{
	this->plugin = plugin;
}

int OilRadius::handle_event()
{
	plugin->config.radius = get_value();
	plugin->send_configure_change();
	return 1;
}


OilIntensity::OilIntensity(OilEffect *plugin, int x, int y)
 : BC_CheckBox(x, y, plugin->config.use_intensity, _("Use intensity"))
{
	this->plugin = plugin;
}

int OilIntensity::handle_event()
{
	plugin->config.use_intensity = get_value();
	plugin->send_configure_change();
	return 1;
}

PLUGIN_THREAD_METHODS

OilWindow::OilWindow(OilEffect *plugin, int x, int y)
 : PluginWindow(plugin, x, y, 300, 160)
{
	x = y = 10;

	add_subwindow(new BC_Title(x, y, _("Radius:")));
	add_subwindow(radius = new OilRadius(plugin, x + 70, y));
	y += 40;
	add_subwindow(intensity = new OilIntensity(plugin, x, y));
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

void OilWindow::update()
{
	radius->update(plugin->config.radius);
	intensity->update(plugin->config.use_intensity);
}

REGISTER_PLUGIN

OilEffect::OilEffect(PluginServer *server)
 : PluginVClient(server)
{
	engine = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

OilEffect::~OilEffect()
{
	delete engine;
	PLUGIN_DESTRUCTOR_MACRO
}

void OilEffect::reset_plugin()
{
	if(engine)
	{
		delete engine;
		engine = 0;
	}
}

PLUGIN_CLASS_METHODS

void OilEffect::load_defaults()
{
	defaults = load_defaults_file("oilpainting.rc");

	config.radius = defaults->get("RADIUS", config.radius);
	config.use_intensity = defaults->get("USE_INTENSITY", config.use_intensity);
}

void OilEffect::save_defaults()
{
	defaults->update("RADIUS", config.radius);
	defaults->update("USE_INTENSITY", config.use_intensity);
	defaults->save();
}

void OilEffect::save_data(KeyFrame *keyframe)
{
	FileXML output;

	output.tag.set_title("OIL_PAINTING");
	output.tag.set_property("RADIUS", config.radius);
	output.tag.set_property("USE_INTENSITY", config.use_intensity);
	output.append_tag();
	output.tag.set_title("/OIL_PAINTING");
	output.append_tag();
	keyframe->set_data(output.string);
}

void OilEffect::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());

	int result = 0;

	while(!input.read_tag())
	{
		if(input.tag.title_is("OIL_PAINTING"))
		{
			config.radius = input.tag.get_property("RADIUS", config.radius);
			config.use_intensity = input.tag.get_property("USE_INTENSITY", config.use_intensity);
		}
	}
}

VFrame *OilEffect::process_tmpframe(VFrame *input)
{
	int cmodel = input->get_color_model();

	switch(cmodel)
	{
	case BC_RGBA16161616:
	case BC_AYUV16161616:
		break;

	default:
		unsupported(cmodel);
		return input;
	}

	if(load_configuration())
		update_gui();

	this->input = input;
	output = input;

	if(!EQUIV(config.radius, 0))
	{
		output = clone_vframe(input);

		if(!engine)
			engine = new OilServer(this, get_project_smp());
		engine->process_packages();
		release_vframe(input);
	}
	return output;
}


OilPackage::OilPackage()
 : LoadPackage()
{
}


OilUnit::OilUnit(OilEffect *plugin, OilServer *server)
 : LoadClient(server)
{
	this->plugin = plugin;
	for(int i = 0; i < 4; i++)
		hist[i] = new int[HIST_SIZE];
	hist2 = new int[HIST_SIZE];
}

OilUnit::~OilUnit()
{
	for(int i = 0; i < 4; i++)
		delete [] hist[i];
	delete [] hist2;
}

void OilUnit::process_package(LoadPackage *package)
{
	OilPackage *pkg = (OilPackage*)package;
	int w = plugin->input->get_w();
	int h = plugin->input->get_h();
	int n = round(plugin->config.radius / 2);
	uint16_t *src, *dest;
	uint16_t val[4];
	int count[4], count2;

	if(!plugin->config.use_intensity)
	{
		for(int y1 = pkg->row1; y1 < pkg->row2; y1++)
		{
			dest = (uint16_t*)plugin->output->get_row_ptr(y1);

			for(int x1 = 0; x1 < w; x1++)
			{
				count[0] = count[1] = count[2] = count[3] = 0;
				val[0] = val[1] = val[2] = val[3] = 0;
				memset(hist[0], 0, sizeof(int) * HIST_SIZE);
				memset(hist[1], 0, sizeof(int) * HIST_SIZE);
				memset(hist[2], 0, sizeof(int) * HIST_SIZE);
				memset(hist[3], 0, sizeof(int) * HIST_SIZE);

				int x3 = CLIP(x1 - n, 0, w - 1);
				int y3 = CLIP(y1 - n, 0, h - 1);
				int x4 = CLIP(x1 + n + 1, 0, w - 1);
				int y4 = CLIP(y1 + n + 1, 0, h - 1);

				for(int y2 = y3; y2 < y4; y2++)
				{
					src = (uint16_t*)plugin->input->get_row_ptr(y2);

					for(int x2 = x3; x2 < x4; x2++)
					{
						int c;
						int subscript;
						uint16_t value;

						value = src[x2 * 4];
						subscript = value >> 8;

						if((c = ++hist[0][subscript]) > count[0])
						{
							val[0] = value;
							count[0] = c;
						}

						value = src[x2 * 4 + 1];
						subscript = value >> 8;

						if((c = ++hist[1][subscript]) > count[1])
						{
							val[1] = value;
							count[1] = c;
						}
						value = src[x2 * 4 + 2];
						subscript = value >> 8;

						if((c = ++hist[2][subscript]) > count[2])
						{
							val[2] = value;
							count[2] = c;
						}

						value = src[x2 * 4 + 3];
						subscript = value >> 8;

						if((c = ++hist[3][subscript]) > count[3])
						{
							val[3] = value;
							count[3] = c;
						}
					}
				}

				dest[x1 * 4] = val[0];
				dest[x1 * 4 + 1] = val[1];
				dest[x1 * 4 + 2] = val[2];
				dest[x1 * 4 + 3] = val[3];
			}
		}
	}
	else
	{
		for(int y1 = pkg->row1; y1 < pkg->row2; y1++)
		{
			dest = (uint16_t*)plugin->output->get_row_ptr(y1);

			for(int x1 = 0; x1 < w; x1++)
			{
				count2 = 0;
				val[0] = val[1] = val[2] = val[3] = 0;
				memset(hist2, 0, sizeof(int) * HIST_SIZE);

				int x3 = CLIP((x1 - n), 0, w - 1);
				int y3 = CLIP((y1 - n), 0, h - 1);
				int x4 = CLIP((x1 + n + 1), 0, w - 1);
				int y4 = CLIP((y1 + n + 1), 0, h - 1);

				for(int y2 = y3; y2 < y4; y2++)
				{
					src = (uint16_t*)plugin->input->get_row_ptr(y2);

					for(int x2 = x3; x2 < x4; x2++)
					{
						int c;
						uint16_t *p = src + x2 * 4;

						int ix = ((unsigned int)p[0] * 77 +
							p[1] * 150 + p[2] * 29) >> 16;

						c = ++hist2[ix];
						if(c > count2)
						{
							val[0] = src[x2 * 4];
							val[1] = src[x2 * 4 + 1];
							val[2] = src[x2 * 4 + 2];
							val[3] = src[x2 * 4 + 3];
							count2 = c;
						}
					}
				}

				dest[x1 * 4] = val[0];
				dest[x1 * 4 + 1] = val[1];
				dest[x1 * 4 + 2] = val[2];
				dest[x1 * 4 + 3] = val[3];
			}
		}
	}
}


OilServer::OilServer(OilEffect *plugin, int cpus)
 : LoadServer(cpus, cpus)
{
	this->plugin = plugin;
}

void OilServer::init_packages()
{
	for(int i = 0; i < get_total_packages(); i++)
	{
		OilPackage *pkg = (OilPackage*)get_package(i);
		pkg->row1 = plugin->input->get_h() * i / LoadServer::get_total_packages();
		pkg->row2 = plugin->input->get_h() * (i + 1) / LoadServer::get_total_packages();
	}
}


LoadClient* OilServer::new_client()
{
	return new OilUnit(plugin, this);
}

LoadPackage* OilServer::new_package()
{
	return new OilPackage;
}
