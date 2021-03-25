// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bctitle.h"
#include "sharpenwindow.h"

PLUGIN_THREAD_METHODS

SharpenWindow::SharpenWindow(SharpenMain *plugin, int x, int y)
 : PluginWindow(plugin,
	x,
	y,
	230,
	150)
{
	x = y = 10;
	add_tool(new BC_Title(x, y, _("Sharpness")));
	y += 20;
	add_tool(sharpen_slider = new SharpenSlider(plugin, &(plugin->config.sharpness), x, y));
	y += 30;
	add_tool(sharpen_interlace = new SharpenInterlace(plugin, x, y));
	y += 30;
	add_tool(sharpen_horizontal = new SharpenHorizontal(plugin, x, y));
	y += 30;
	add_tool(sharpen_luminance = new SharpenLuminance(plugin, x, y));
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

void SharpenWindow::update()
{
	sharpen_slider->update((int)plugin->config.sharpness);
	sharpen_interlace->update(plugin->config.interlace);
	sharpen_horizontal->update(plugin->config.horizontal);
	sharpen_luminance->update(plugin->config.luminance);
}


SharpenSlider::SharpenSlider(SharpenMain *client, double *output, int x, int y)
 : BC_ISlider(x, 
	y,
	0,
	200, 
	200,
	0, 
	MAXSHARPNESS, 
	(int)*output, 
	0, 
	0, 
	0)
{
	this->client = client;
	this->output = output;
}

int SharpenSlider::handle_event()
{
	*output = get_value();
	client->send_configure_change();
	return 1;
}


SharpenInterlace::SharpenInterlace(SharpenMain *client, int x, int y)
 : BC_CheckBox(x, y, client->config.interlace, _("Interlace"))
{
	this->client = client;
}

int SharpenInterlace::handle_event()
{
	client->config.interlace = get_value();
	client->send_configure_change();
	return 1;
}


SharpenHorizontal::SharpenHorizontal(SharpenMain *client, int x, int y)
 : BC_CheckBox(x, y, client->config.horizontal, _("Horizontal only"))
{
	this->client = client;
}

int SharpenHorizontal::handle_event()
{
	client->config.horizontal = get_value();
	client->send_configure_change();
	return 1;
}


SharpenLuminance::SharpenLuminance(SharpenMain *client, int x, int y)
 : BC_CheckBox(x, y, client->config.luminance, _("Luminance only"))
{
	this->client = client;
}

int SharpenLuminance::handle_event()
{
	client->config.luminance = get_value();
	client->send_configure_change();
	return 1;
}
