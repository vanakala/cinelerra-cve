// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2018 Einar Rünkaru <einarrunkaru at gmail dot com>

// Test of colormodels conversion

#include "bchash.h"
#include "bctitle.h"
#include "colormodels.h"
#include "colorspace.h"
#include "colorspaces.h"
#include "filexml.h"
#include "picon_png.h"
#include "tmpframecache.h"
#include "vframe.h"

#include <stdint.h>
#include <string.h>

// Registers plugin
REGISTER_PLUGIN

ColorSpaceConfig::ColorSpaceConfig()
{
	onoff = 0;
	avlibs = 0;
}

ColorSpaceSwitch::ColorSpaceSwitch(ColorSpace *plugin,
	int x,
	int y)
 : BC_Radial(x, y, plugin->config.onoff, _("On"))
{
	this->plugin = plugin;
}

int ColorSpaceSwitch::handle_event()
{
	plugin->config.onoff = plugin->config.onoff ? 0 : 1;
	plugin->send_configure_change();
	update(plugin->config.onoff, 1);
	return 1;
}

AVlibsSwitch::AVlibsSwitch(ColorSpace *plugin,
	int x,
	int y)
 : BC_Radial(x, y, plugin->config.avlibs, _("Use avlibs"))
{
	this->plugin = plugin;
}

int AVlibsSwitch::handle_event()
{
	plugin->config.avlibs = plugin->config.avlibs ? 0 : 1;
	plugin->send_configure_change();
	update(plugin->config.avlibs, 1);
	return 1;
}


// The gui of the plugin
ColorSpaceWindow::ColorSpaceWindow(ColorSpace *plugin, int x, int y)
 : PluginWindow(plugin,
	x,
	y,
	200,
	150)
{
	x = y = 10;

	add_subwindow(new BC_Title(x, y, _("Status:")));
	add_subwindow(toggle = new ColorSpaceSwitch(plugin,
		x + 60,
		y));
	y += toggle->get_h() + 10;
	add_subwindow(avltoggle = new AVlibsSwitch(plugin,
		x + 60,
		y));
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

void ColorSpaceWindow::update()
{
	toggle->update(plugin->config.onoff);
	avltoggle->update(plugin->config.avlibs);
}

PLUGIN_THREAD_METHODS

ColorSpace::ColorSpace(PluginServer *server)
 : PluginVClient(server)
{
	PLUGIN_CONSTRUCTOR_MACRO
}

ColorSpace::~ColorSpace()
{
	PLUGIN_DESTRUCTOR_MACRO
}

PLUGIN_CLASS_METHODS

void ColorSpace::load_defaults()
{
// Loads defaults from the named rc-file
	defaults = load_defaults_file("colorspace.rc");

	config.onoff = defaults->get("ONOFF", config.onoff);
	config.avlibs = defaults->get("AVLIBS", config.avlibs);
}

void ColorSpace::save_defaults()
{
	defaults->update("ONOFF", config.onoff);
	defaults->update("AVLIBS", config.avlibs);
	defaults->save();
}

// Saves the values of the parameters to the project
void ColorSpace::save_data(KeyFrame *keyframe)
{
	FileXML output;

	output.tag.set_title("COLORSPACE");
	output.tag.set_property("ONOFF", config.onoff);
	output.tag.set_property("AVLIBS", config.avlibs);
	output.append_tag();
	output.tag.set_title("/COLORSPACE");
	output.append_tag();
	keyframe->set_data(output.string);
}

void ColorSpace::read_data(KeyFrame *keyframe)
{
	FileXML input;

	if(!keyframe)
		return;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());

	while(!input.read_tag())
	{
		if(input.tag.title_is("COLORSPACE"))
		{
			config.onoff = input.tag.get_property("ONOFF", config.onoff);
			config.avlibs = input.tag.get_property("AVLIBS", config.avlibs);
		}
	}
}

// Loads parameters from keyframe
// Keyframe may be missing
int ColorSpace::load_configuration()
{
	int prev_val = config.onoff;
	KeyFrame *prev_keyframe = get_prev_keyframe(source_pts);

	if(prev_keyframe)
	{
		read_data(prev_keyframe);
		return !(prev_val == config.onoff);
	}
	return 0;
}

VFrame *ColorSpace::process_tmpframe(VFrame *frame)
{
	VFrame *tmp;

	if(load_configuration())
		update_gui();

	if(config.onoff)
	{
		if(config.avlibs)
		{
			switch(frame->get_color_model())
			{
			case BC_AYUV16161616:
				tmp = BC_Resources::tmpframes.get_tmpframe(
					frame->get_w(),
					frame->get_h(),
					BC_RGBA16161616);
				tmp->transfer_from(frame);
				frame->transfer_from(tmp);
				release_vframe(tmp);
				break;
			case BC_RGBA16161616:
				tmp = BC_Resources::tmpframes.get_tmpframe(
					frame->get_w(),
					frame->get_h(),
					BC_AYUV16161616);
				tmp->transfer_from(frame);
				frame->transfer_from(tmp);
				release_vframe(tmp);
				break;
			default:
				unsupported(frame->get_color_model());
				break;
			}
		}
		else
		{
			int width = frame->get_w();
			int height = frame->get_h();
			int r, g, b, y, u, v, a, k;

			switch(frame->get_color_model())
			{
			case BC_AYUV16161616:
				tmp = BC_Resources::tmpframes.get_tmpframe(
					width, height,
					BC_RGBA16161616);
				for(int j = 0; j < height; j++)
				{
					uint16_t *frow = (uint16_t*)frame->get_row_ptr(j);
					uint16_t *trow = (uint16_t*)tmp->get_row_ptr(j);
					for(int i = 0; i < width; i++)
					{
						k = 4 * i;
						a = frow[k + 0];
						y = frow[k + 1];
						u = frow[k + 2];
						v = frow[k + 3];
						ColorSpaces::yuv_to_rgb_16(r, g, b, y, u, v);
						trow[k + 0] = r;
						trow[k + 1] = g;
						trow[k + 2] = b;
						trow[k + 3] = a;
					}
				}
				for(int j = 0; j < height; j++)
				{
					uint16_t *frow = (uint16_t*)frame->get_row_ptr(j);
					uint16_t *trow = (uint16_t*)tmp->get_row_ptr(j);
					for(int i = 0; i < width; i++)
					{
						k = 4 * i;
						r = trow[k + 0];
						g = trow[k + 1];
						b = trow[k + 2];
						a = trow[k + 3];
						ColorSpaces::rgb_to_yuv_16(r, g, b, y, u, v);
						frow[k + 0] = a;
						frow[k + 1] = y;
						frow[k + 2] = u;
						frow[k + 3] = v;
					}
				}
				release_vframe(tmp);
				break;
			case BC_RGBA16161616:
				tmp = BC_Resources::tmpframes.get_tmpframe(
					width, height,
					BC_AYUV16161616);
				for(int j = 0; j < height; j++)
				{
					uint16_t *frow = (uint16_t*)frame->get_row_ptr(j);
					uint16_t *trow = (uint16_t*)tmp->get_row_ptr(j);
					for(int i = 0; i < width; i++)
					{
						k = 4 * i;
						r = frow[k + 0];
						g = frow[k + 1];
						b = frow[k + 2];
						a = frow[k + 3];
						ColorSpaces::rgb_to_yuv_16(r, g, b, y, u, v);
						trow[k + 0] = a;
						trow[k + 1] = y;
						trow[k + 2] = u;
						trow[k + 3] = v;
					}
				}
				for(int j = 0; j < height; j++)
				{
					uint16_t *frow = (uint16_t*)frame->get_row_ptr(j);
					uint16_t *trow = (uint16_t*)tmp->get_row_ptr(j);
					for(int i = 0; i < width; i++)
					{
						k = 4 * i;
						a = trow[k + 0];
						y = trow[k + 1];
						u = trow[k + 2];
						v = trow[k + 3];
						ColorSpaces::yuv_to_rgb_16(r, g, b, y, u, v);
						frow[k + 0] = r;
						frow[k + 1] = g;
						frow[k + 2] = b;
						frow[k + 3] = a;
					}
				}
				release_vframe(tmp);
				break;
			default:
				unsupported(frame->get_color_model());
				break;
			}
		}
	}
	frame->set_transparent();
	return frame;
}
