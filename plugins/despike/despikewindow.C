// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bctitle.h"
#include "despikewindow.h"
#include "language.h"

#include <string.h>

PLUGIN_THREAD_METHODS

DespikeWindow::DespikeWindow(Despike *plugin, int x, int y)
 : PluginWindow(plugin,
	x,
	y, 
	230, 
	110)
{
	x = y = 10;
	add_tool(new BC_Title(5, y, _("Maximum level:")));
	y += 20;
	add_tool(level = new DespikeLevel(plugin, x, y));
	y += 30;
	add_tool(new BC_Title(5, y, _("Maximum rate of change:")));
	y += 20;
	add_tool(slope = new DespikeSlope(plugin, x, y));
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

void DespikeWindow::update()
{
	level->update(plugin->config.level);
	slope->update(plugin->config.slope);
}


DespikeLevel::DespikeLevel(Despike *despike, int x, int y)
 : BC_FSlider(x, 
	y,
	0,
	200,
	200,
	INFINITYGAIN, 
	0,
	despike->config.level)
{
	this->despike = despike;
}

int DespikeLevel::handle_event()
{
	despike->config.level = get_value();
	despike->send_configure_change();
	return 1;
}


DespikeSlope::DespikeSlope(Despike *despike, int x, int y)
 : BC_FSlider(x, 
	y,
	0,
	200,
	200,
	INFINITYGAIN, 
	0,
	despike->config.slope)
{
	this->despike = despike;
}

int DespikeSlope::handle_event()
{
	despike->config.slope = get_value();
	despike->send_configure_change();
	return 1;
}
