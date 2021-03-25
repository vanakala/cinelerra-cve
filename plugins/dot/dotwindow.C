// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bctitle.h"
#include "dotwindow.h"
#include "language.h"
#include "pluginclient.h"

PLUGIN_THREAD_METHODS


DotWindow::DotWindow(DotMain *plugin, int x, int y)
 : PluginWindow(plugin,
	x,
	y,
	230,
	170)
{
	BC_WindowBase *win;
	int title_h;

	x = 10;
	y = 10;

	add_subwindow(win = new BC_Title(x, y, _("Dot depth")));
	title_h = win->get_h() + 8;
	y += title_h;
	add_subwindow(dot_depth = new DotSize(plugin, x, y,
		&plugin->config.dot_depth, 1, 8));
	y += dot_depth->get_h() + 8;
	add_subwindow(win = new BC_Title(x, y, _("Dot size")));
	y += title_h;
	add_subwindow(dot_size = new DotSize(plugin, x, y,
		&plugin->config.dot_size, 4, 16));
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

void DotWindow::update()
{
	dot_depth->update(plugin->config.dot_depth);
	dot_size->update(plugin->config.dot_size);
}

DotSize::DotSize(DotMain *plugin, int x, int y,
	int *output, int min, int max)
 : BC_ISlider(x, y, 0, 200, 200, min, max, *output)
{
	this->plugin = plugin;
	this->output = output;
}

int DotSize::handle_event()
{
	*output = get_value();
	plugin->send_configure_change();
	return 1;
}
