// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bctitle.h"
#include "brightnesswindow.h"

PLUGIN_THREAD_METHODS


BrightnessWindow::BrightnessWindow(BrightnessMain *plugin, int x, int y)
 : PluginWindow(plugin, x,
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
	double fraction;

	if(is_brightness)
	{
		fraction = *output / 100.0;
	}
	else
	{
		fraction = (*output < 0) ? 
			(*output + 100.0) / 100.0 :
			(*output + 25.0) / 25.0;
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
