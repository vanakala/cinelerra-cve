// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "language.h"
#include "rgb601window.h"


PLUGIN_THREAD_METHODS


RGB601Window::RGB601Window(RGB601Main *plugin, int x, int y)
 : PluginWindow(plugin,
	x,
	y,
	235,
	100)
{
	x = y = 10;

	add_tool(forward = new RGB601Direction(this, 
		x, 
		y, 
		&plugin->config.direction, 
		RGB601_FORW,
		_("RGB -> 601 compression")));
	y += 30;
	add_tool(reverse = new RGB601Direction(this, 
		x, 
		y, 
		&plugin->config.direction, 
		RGB601_RVRS,
		_("601 -> RGB expansion")));
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

void RGB601Window::update()
{
	forward->update(plugin->config.direction == RGB601_FORW);
	reverse->update(plugin->config.direction == RGB601_RVRS);
}


RGB601Direction::RGB601Direction(RGB601Window *window, int x, int y, int *output, int true_value, const char *text)
 : BC_CheckBox(x, y, *output == true_value, text)
{
	this->output = output;
	this->true_value = true_value;
	this->window = window;
}

int RGB601Direction::handle_event()
{
	*output = get_value() ? true_value : 0;
	window->update();
	window->plugin->send_configure_change();
	return 1;
}
