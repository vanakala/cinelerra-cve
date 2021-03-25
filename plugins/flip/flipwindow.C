// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "flipwindow.h"

PLUGIN_THREAD_METHODS


FlipWindow::FlipWindow(FlipMain *plugin, int x, int y)
 : PluginWindow(plugin,
	x,
	y,
	250,
	100)
{
	x = 10;
	y = 10;
	add_tool(flip_vertical = new FlipToggle(plugin,
		&(plugin->config.flip_vertical), 
		_("Vertical"),
		x, 
		y));
	y += 30;
	add_tool(flip_horizontal = new FlipToggle(plugin,
		&(plugin->config.flip_horizontal), 
		_("Horizontal"),
		x, 
		y));
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

void FlipWindow::update()
{
	flip_vertical->update(plugin->config.flip_vertical);
	flip_horizontal->update(plugin->config.flip_horizontal);
}


FlipToggle::FlipToggle(FlipMain *client, int *output, char *string, int x, int y)
 : BC_CheckBox(x, y, *output, string)
{
	this->client = client;
	this->output = output;
}

int FlipToggle::handle_event()
{
	*output = get_value();
	client->send_configure_change();
	return 1;
}
