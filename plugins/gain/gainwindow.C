// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bchash.h"
#include "bctitle.h"
#include "gainwindow.h"
#include "language.h"

PLUGIN_THREAD_METHODS


GainWindow::GainWindow(Gain *plugin, int x, int y)
 : PluginWindow(plugin->gui_string,
	x,
	y, 
	230, 
	60)
{
	x = y = 10;
	add_tool(new BC_Title(5, y, _("Level:")));
	y += 20;
	add_tool(level = new GainLevel(plugin, x, y));
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

void GainWindow::update()
{
	level->update(plugin->config.level);
}

GainLevel::GainLevel(Gain *plugin, int x, int y)
 : BC_FSlider(x, 
	y,
	0,
	200,
	200,
	INFINITYGAIN, 
	40,
	plugin->config.level)
{
	this->plugin = plugin;
}

int GainLevel::handle_event()
{
	plugin->config.level = get_value();
	plugin->send_configure_change();
	return 1;
}
