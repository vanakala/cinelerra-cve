// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bchash.h"
#include "bcslider.h"
#include "bctitle.h"
#include "clip.h"
#include "colorspaces.h"
#include "filexml.h"
#include "picon_png.h"
#include "pluginvclient.h"
#include "pluginwindow.h"
#include "vframe.h"
#include "yuv.h"

#include <stdint.h>
#include <string.h>

REGISTER_PLUGIN

YUVConfig::YUVConfig()
{
	y = 0;
	u = 0;
	v = 0;
}

void YUVConfig::copy_from(YUVConfig &src)
{
	y = src.y;
	u = src.u;
	v = src.v;
}

int YUVConfig::equivalent(YUVConfig &src)
{
	return EQUIV(y, src.y) && EQUIV(u, src.u) && EQUIV(v, src.v);
}

void YUVConfig::interpolate(YUVConfig &prev, 
	YUVConfig &next, 
	ptstime prev_pts,
	ptstime next_pts,
	ptstime current_pts)
{
	PLUGIN_CONFIG_INTERPOLATE_MACRO

	y = prev.y * prev_scale + next.y * next_scale;
	u = prev.u * prev_scale + next.u * next_scale;
	v = prev.v * prev_scale + next.v * next_scale;
}


#define MAXVALUE 100

YUVLevel::YUVLevel(YUVEffect *plugin, double *output, int x, int y)
 : BC_FSlider(x, 
			y,
			0,
			200, 
			200, 
			-MAXVALUE, 
			MAXVALUE, 
			*output)
{
	this->plugin = plugin;
	this->output = output;
}

int YUVLevel::handle_event()
{
	*output = get_value();
	plugin->send_configure_change();
	return 1;
}


YUVWindow::YUVWindow(YUVEffect *plugin, int x, int y)
 : PluginWindow(plugin,
	x,
	y, 
	260, 
	100)
{
	int x1 = 50;

	x = y = 10;
	add_subwindow(new BC_Title(x, y, _("Y:")));
	add_subwindow(this->y = new YUVLevel(plugin, &plugin->config.y, x1, y));
	y += 30;
	add_subwindow(new BC_Title(x, y, _("U:")));
	add_subwindow(u = new YUVLevel(plugin, &plugin->config.u, x1, y));
	y += 30;
	add_subwindow(new BC_Title(x, y, _("V:")));
	add_subwindow(v = new YUVLevel(plugin, &plugin->config.v, x1, y));
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

void YUVWindow::update()
{
	y->update(plugin->config.y);
	u->update(plugin->config.u);
	v->update(plugin->config.v);
}

PLUGIN_THREAD_METHODS

YUVEffect::YUVEffect(PluginServer *server)
 : PluginVClient(server)
{
	PLUGIN_CONSTRUCTOR_MACRO
}

YUVEffect::~YUVEffect()
{
	PLUGIN_DESTRUCTOR_MACRO
}

PLUGIN_CLASS_METHODS


void YUVEffect::load_defaults()
{
	defaults = load_defaults_file("yuv.rc");

	config.y = defaults->get("Y", config.y);
	config.u = defaults->get("U", config.u);
	config.v = defaults->get("V", config.v);
}

void YUVEffect::save_defaults()
{
	defaults->update("Y", config.y);
	defaults->update("U", config.u);
	defaults->update("V", config.v);
	defaults->save();
}

void YUVEffect::save_data(KeyFrame *keyframe)
{
	FileXML output;

	output.tag.set_title("YUV");
	output.tag.set_property("Y", config.y);
	output.tag.set_property("U", config.u);
	output.tag.set_property("V", config.v);
	output.append_tag();
	output.tag.set_title("/YUV");
	output.append_tag();
	keyframe->set_data(output.string);
}

void YUVEffect::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());
	while(!input.read_tag())
	{
		if(input.tag.title_is("YUV"))
		{
			config.y = input.tag.get_property("Y", config.y);
			config.u = input.tag.get_property("U", config.u);
			config.v = input.tag.get_property("V", config.v);
		}
	}
}

VFrame *YUVEffect::process_tmpframe(VFrame *input)
{
	if(load_configuration())
		update_gui();

	if(!EQUIV(config.y, 0) || !EQUIV(config.u, 0) || !EQUIV(config.v, 0))
	{
		int w = input->get_w();
		int h = input->get_h();

		double y_scale = (config.y + MAXVALUE) / MAXVALUE;
		double u_scale = (config.u + MAXVALUE) / MAXVALUE;
		double v_scale = (config.v + MAXVALUE) / MAXVALUE;

		if(u_scale > 1) u_scale = 1 + (u_scale - 1) * 4;
		if(v_scale > 1) v_scale = 1 + (v_scale - 1) * 4;

		switch(input->get_color_model())
		{
		case BC_RGBA16161616:
			for(int i = 0; i < h; i++)
			{
				uint16_t *in_row = (uint16_t*)input->get_row_ptr(i);

				for(int j = 0; j < w; j++)
				{
					int y, u, v, r, g, b;

					ColorSpaces::rgb_to_yuv_16(in_row[0], in_row[1], in_row[2], y, u, v);

					y = round((double)y * y_scale);
					u = round((double)(u - 0x8000) * u_scale);
					v = round((double)(v - 0x8000) * v_scale);
					u += 0x8000;
					v += 0x8000;
					CLAMP(y, 0, 0xffff);
					CLAMP(u, 0, 0xffff);
					CLAMP(v, 0, 0xffff);
					ColorSpaces::yuv_to_rgb_16(r, g, b, y, u, v);

					in_row[0] = r;
					in_row[1] = g;
					in_row[2] = b;

					in_row += 4;
				}
			}
			break;

		case BC_AYUV16161616:
			for(int i = 0; i < input->get_h(); i++)
			{
				uint16_t *in_row = (uint16_t*)input->get_row_ptr(i);

				for(int j = 0; j < w; j++)
				{
					int y = round((double)in_row[1] * y_scale);
					int u = round((double)(in_row[2] - 0x8000) * u_scale);
					int v = round((double)(in_row[3] - 0x8000) * v_scale);

					u += 0x8000;
					v += 0x8000;
					in_row[1] = CLIP(y, 0, 0xffff);
					in_row[2] = CLIP(u, 0, 0xffff);
					in_row[3] = CLIP(v, 0, 0xffff);

					in_row += 4;
				}
			}
			break;
		default:
			unsupported(input->get_color_model());
			break;
		}
	}
	return input;
}
