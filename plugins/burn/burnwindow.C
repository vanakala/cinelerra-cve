// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bctitle.h"
#include "burnwindow.h"

PLUGIN_THREAD_METHODS


BurnWindow::BurnWindow(BurnMain *plugin, int x, int y)
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

	add_subwindow(win = new BC_Title(x, y, _("Threshold")));
	title_h = win->get_h() + 8;
	y += title_h;
	add_subwindow(threshold = new BurnSize(plugin, x, y,
		&plugin->config.threshold, 0, 100));
	y += threshold->get_h() + 8;
	add_subwindow(win = new BC_Title(x, y, _("Decay")));
	y += title_h;
	add_subwindow(decay = new BurnSize(plugin, x, y,
		&plugin->config.decay, 0, 50));
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

void BurnWindow::update()
{
	threshold->update(plugin->config.threshold);
	decay->update(plugin->config.decay);
}

BurnSize::BurnSize(BurnMain *plugin, int x, int y,
	int *output, int min, int max)
 : BC_ISlider(x, y, 0, 200, 200, min, max, *output)
{
	this->plugin = plugin;
	this->output = output;
}

int BurnSize::handle_event()
{
	*output = get_value();
	plugin->send_configure_change();
	return 1;
}
