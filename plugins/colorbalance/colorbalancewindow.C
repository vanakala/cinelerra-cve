// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bctitle.h"
#include "clip.h"
#include "colorbalancewindow.h"

PLUGIN_THREAD_METHODS


ColorBalanceWindow::ColorBalanceWindow(ColorBalanceMain *plugin, int x, int y)
 : PluginWindow(plugin, x,
	y,
	330, 
	250)
{
	x = 10;
	y = 10;

	add_tool(new BC_Title(x, y, _(plugin->plugin_title())));
	y += 25;
	add_tool(new BC_Title(x, y, _("Cyan")));
	add_tool(cyan = new ColorBalanceSlider(plugin, &(plugin->config.cyan), x + 70, y));
	add_tool(new BC_Title(x + 270, y, _("Red")));
	y += 25;
	add_tool(new BC_Title(x, y, _("Magenta")));
	add_tool(magenta = new ColorBalanceSlider(plugin, &(plugin->config.magenta), x + 70, y));
	add_tool(new BC_Title(x + 270, y, _("Green")));
	y += 25;
	add_tool(new BC_Title(x, y, _("Yellow")));
	add_tool(yellow = new ColorBalanceSlider(plugin, &(plugin->config.yellow), x + 70, y));
	add_tool(new BC_Title(x + 270, y, _("Blue")));
	y += 25;
	add_tool(preserve = new ColorBalancePreserve(plugin, x + 70, y));
	y += preserve->get_h() + 10;
	add_tool(lock_params = new ColorBalanceLock(plugin, x + 70, y));
	y += lock_params->get_h() + 10;
	add_tool(new ColorBalanceWhite(plugin, this, x, y));
	y += lock_params->get_h() + 10;
	add_tool(new ColorBalanceReset(plugin, this, x, y));

	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

void ColorBalanceWindow::update()
{
	cyan->update(plugin->config.cyan);
	magenta->update(plugin->config.magenta);
	yellow->update(plugin->config.yellow);
	preserve->update(plugin->config.preserve);
	lock_params->update(plugin->config.lock_params);
}


ColorBalanceSlider::ColorBalanceSlider(ColorBalanceMain *client,
	double *output, int x, int y)
 : BC_ISlider(x, 
	y, 
	0, 
	200, 
	200,
	-1000, 
	1000, 
	(int)*output)
{
	this->client = client;
	this->output = output;
}

int ColorBalanceSlider::handle_event()
{
	double difference = get_value() - *output;

	*output = get_value();
	if(!EQUIV(difference, 0))
	{
		client->synchronize_params(this, difference);
		client->send_configure_change();
	}
	return 1;
}

char* ColorBalanceSlider::get_caption()
{
	double fraction = client->calculate_transfer(*output);

	sprintf(string, "%0.4f", fraction);
	return string;
}


ColorBalancePreserve::ColorBalancePreserve(ColorBalanceMain *client, int x, int y)
 : BC_CheckBox(x, 
	y,
	client->config.preserve, 
	_("Preserve luminosity"))
{
	this->client = client;
}

int ColorBalancePreserve::handle_event()
{
	client->config.preserve = get_value();
	client->send_configure_change();
	return 1;
}

ColorBalanceLock::ColorBalanceLock(ColorBalanceMain *client, int x, int y)
 : BC_CheckBox(x, 
	y, 
	client->config.lock_params, 
	_("Lock parameters"))
{
	this->client = client;
}

int ColorBalanceLock::handle_event()
{
	client->config.lock_params = get_value();
	client->send_configure_change();
	return 1;
}


ColorBalanceWhite::ColorBalanceWhite(ColorBalanceMain *plugin, 
	ColorBalanceWindow *gui,
	int x, 
	int y)
 : BC_GenericButton(x, y, _("White balance"))
{
	this->plugin = plugin;
	this->gui = gui;
}

int ColorBalanceWhite::handle_event()
{
	int red, green, blue;
// Get colorpicker value
	plugin->get_picker_rgb(&red, &green, &blue);
// Try normalizing to green like others do it
	double r_factor = (double)green / red;
	double g_factor = 1.0;
	double b_factor = (double)green / blue;
// Convert factors to slider values
	plugin->config.cyan = plugin->calculate_slider(r_factor);
	plugin->config.magenta = plugin->calculate_slider(g_factor);
	plugin->config.yellow = plugin->calculate_slider(b_factor);
	gui->update();
	plugin->send_configure_change();
	return 1;
}


ColorBalanceReset::ColorBalanceReset(ColorBalanceMain *plugin, 
	ColorBalanceWindow *gui, 
	int x, 
	int y)
 : BC_GenericButton(x, y, _("Reset"))
{
	this->plugin = plugin;
	this->gui = gui;
}

int ColorBalanceReset::handle_event()
{
	plugin->config.cyan = 0;
	plugin->config.magenta = 0;
	plugin->config.yellow = 0;
	gui->update();
	plugin->send_configure_change();
	return 1;
}
