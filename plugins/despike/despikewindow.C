
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
#include "despikewindow.h"
#include "language.h"

#include <string.h>

PLUGIN_THREAD_METHODS

DespikeWindow::DespikeWindow(Despike *plugin, int x, int y)
 : BC_Window(plugin->gui_string, 
	x,
	y, 
	230, 
	110, 
	230, 
	110, 
	0, 
	0,
	1)
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

DespikeWindow::~DespikeWindow()
{
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
