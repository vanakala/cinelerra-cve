
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

#include "bctitle.h"
#include "brightnesswindow.h"

PLUGIN_THREAD_METHODS


BrightnessWindow::BrightnessWindow(BrightnessMain *plugin, int x, int y)
 : PluginWindow(plugin->gui_string, x,
	y,
	330, 
	160)
{
	x = 10;
	y = 10;

	add_tool(new BC_Title(x, y, _(plugin->plugin_title())));
	y += 25;
	add_tool(new BC_Title(x, y,_("Brightness:")));
	add_tool(brightness = new BrightnessSlider(plugin,
		&(plugin->config.brightness),
		x + 80, 
		y,
		1));
	y += 25;
	add_tool(new BC_Title(x, y, _("Contrast:")));
	add_tool(contrast = new BrightnessSlider(plugin,
		&(plugin->config.contrast),
		x + 80, 
		y,
		0));
	y += 30;
	add_tool(luma = new BrightnessLuma(plugin,
		x, 
		y));
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

void BrightnessWindow::update()
{
	brightness->update(plugin->config.brightness);
	contrast->update(plugin->config.contrast);
	luma->update(plugin->config.luma);
}


BrightnessSlider::BrightnessSlider(BrightnessMain *client, 
	double *output,
	int x, int y,
	int is_brightness)
 : BC_FSlider(x, y,
	0, 
	200, 
	200,
	-100, 
	100, 
	*output)
{
	this->client = client;
	this->output = output;
	this->is_brightness = is_brightness;
}

int BrightnessSlider::handle_event()
{
	*output = get_value();
	client->send_configure_change();
	return 1;
}

char* BrightnessSlider::get_caption()
{
	float fraction;
	if(is_brightness)
	{
		fraction = *output / 100;
	}
	else
	{
		fraction = (*output < 0) ? 
			(*output + 100) / 100 : 
			(*output + 25) / 25;
	}
	sprintf(string, "%0.4f", fraction);
	return string;
}


BrightnessLuma::BrightnessLuma(BrightnessMain *client, 
	int x, 
	int y)
 : BC_CheckBox(x, 
	y,
	client->config.luma,
	_("Boost luminance only"))
{
	this->client = client;
}

int BrightnessLuma::handle_event()
{
	client->config.luma = get_value();
	client->send_configure_change();
	return 1;
}
