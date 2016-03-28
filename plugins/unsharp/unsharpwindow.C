
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
#include "language.h"
#include "unsharp.h"
#include "unsharpwindow.h"


PLUGIN_THREAD_METHODS


UnsharpWindow::UnsharpWindow(UnsharpMain *plugin, int x, int y)
 : PluginWindow(plugin->gui_string, 
	x,
	y,
	285,
	160)
{
	BC_Title *title;
	x = y = 10;

	add_subwindow(title = new BC_Title(x, y + 10, _("Radius:")));
	add_subwindow(radius = new UnsharpRadius(plugin, 
		x + title->get_w() + 10, 
		y));

	y += 40;
	add_subwindow(title = new BC_Title(x, y + 10, _("Amount:")));
	add_subwindow(amount = new UnsharpAmount(plugin, 
		x + title->get_w() + 10, 
		y));

	y += 40;
	add_subwindow(title = new BC_Title(x, y + 10, _("Threshold:")));
	add_subwindow(threshold = new UnsharpThreshold(plugin, 
		x + title->get_w() + 10, 
		y));

	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

UnsharpWindow::~UnsharpWindow()
{
}

void UnsharpWindow::update()
{
	radius->update(plugin->config.radius);
	amount->update(plugin->config.amount);
	threshold->update(plugin->config.threshold);
}


UnsharpRadius::UnsharpRadius(UnsharpMain *plugin, int x, int y)
 : BC_FPot(x, y, plugin->config.radius, 0.1, 120)
{
	this->plugin = plugin;
}

int UnsharpRadius::handle_event()
{
	plugin->config.radius = get_value();
	plugin->send_configure_change();
	return 1;
}

UnsharpAmount::UnsharpAmount(UnsharpMain *plugin, int x, int y)
 : BC_FPot(x, y, plugin->config.amount, 0, 5)
{
	this->plugin = plugin;
}

int UnsharpAmount::handle_event()
{
	plugin->config.amount = get_value();
	plugin->send_configure_change();
	return 1;
}

UnsharpThreshold::UnsharpThreshold(UnsharpMain *plugin, int x, int y)
 : BC_IPot(x, y, plugin->config.threshold, 0, 255)
{
	this->plugin = plugin;
}

int UnsharpThreshold::handle_event()
{
	plugin->config.threshold = get_value();
	plugin->send_configure_change();
	return 1;
}
