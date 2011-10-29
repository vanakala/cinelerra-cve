
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

#include "bcdisplayinfo.h"
#include "sharpenwindow.h"

PLUGIN_THREAD_METHODS

SharpenWindow::SharpenWindow(SharpenMain *plugin, int x, int y)
 : BC_Window(plugin->gui_string, 
	x,
	y,
	210, 
	120, 
	210, 
	120, 
	0, 
	0,
	1)
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

SharpenWindow::~SharpenWindow()
{
}

void SharpenWindow::update()
{
	sharpen_slider->update((int)plugin->config.sharpness);
	sharpen_interlace->update(plugin->config.interlace);
	sharpen_horizontal->update(plugin->config.horizontal);
	sharpen_luminance->update(plugin->config.luminance);
}


SharpenSlider::SharpenSlider(SharpenMain *client, float *output, int x, int y)
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

SharpenSlider::~SharpenSlider()
{
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

SharpenInterlace::~SharpenInterlace()
{
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

SharpenHorizontal::~SharpenHorizontal()
{
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

SharpenLuminance::~SharpenLuminance()
{
}

int SharpenLuminance::handle_event()
{
	client->config.luminance = get_value();
	client->send_configure_change();
	return 1;
}
