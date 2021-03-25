// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bctitle.h"
#include "language.h"
#include "holowindow.h"


PLUGIN_THREAD_METHODS


HoloWindow::HoloWindow(HoloMain *plugin, int x, int y)
 : PluginWindow(plugin,
	x,
	y,
	300, 
	170)
{
	BC_WindowBase *win;
	int title_h;
	x = 10;
	y = 10;

	add_subwindow(win = new BC_Title(x, y, _("Threshold")));
	title_h = win->get_h() + 8;
	y += title_h;
	add_subwindow(threshold = new HoloSize(plugin, x, y,
		&plugin->config.threshold, 1, 100));
	y+= threshold->get_h() + 8;
	add_subwindow(new BC_Title(x, y, _("Recycle")));
	y += title_h;
	add_subwindow(recycle = new HoloTime(plugin, x, y,
		&plugin->config.recycle, 0.01, 4));
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

void HoloWindow::update()
{
	threshold->update(plugin->config.threshold);
	recycle->update(plugin->config.recycle);
}

HoloSize::HoloSize(HoloMain *plugin, int x, int y,
        int *output, int min, int max)
 : BC_ISlider(x, y, 0, 200, 200, min, max, *output)
{
	this->plugin = plugin;
	this->output = output;
}

int HoloSize::handle_event()
{
	*output = get_value();
	plugin->send_configure_change();
	return 1;
}

HoloTime::HoloTime(HoloMain *plugin, int x, int y,
        double *output, double min, double max)
 : BC_FSlider(x, y, 0, 200, 200, min, max, *output)
{
	this->plugin = plugin;
	this->output = output;
}

int HoloTime::handle_event()
{
	*output = get_value();
	plugin->send_configure_change();
	return 1;
}
