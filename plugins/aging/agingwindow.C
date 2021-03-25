// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bctitle.h"
#include "bcsignals.h"
#include "agingwindow.h"

PLUGIN_THREAD_METHODS

AgingWindow::AgingWindow(AgingMain *plugin, int x, int y)
 : PluginWindow(plugin, x, y, 230, 245)
{
	BC_WindowBase *win;
	int title_h;

	x = 10;
	y = 10;

	add_subwindow(colorage = new AgingToggle(plugin, x, y, &plugin->config.colorage,
		_("Colorage")));
	y += colorage->get_h() + 8;
	add_subwindow(scratch = new AgingToggle(plugin, x, y, &plugin->config.scratch,
		_("Scratch")));
	y += scratch->get_h() + 8;
	add_subwindow(win = new BC_Title(x, y, _("Scratch lines:")));
	title_h = win->get_h() + 8;
	y += title_h;
	add_subwindow(scratch_lines = new AgingSize(plugin, x, y, &plugin->config.scratch_lines,
		0, SCRATCH_MAX));
	y += scratch_lines->get_h() + 8;
	add_subwindow(pits = new AgingToggle(plugin, x, y, &plugin->config.pits,
		_("Pits")));
	y += pits->get_h() + 8;
	add_subwindow(new BC_Title(x, y, _("Area scale:")));
	y += title_h;
	add_subwindow(area_scale = new AgingSize(plugin, x, y, &plugin->config.area_scale,
		0, AREA_SCALE_MAX));
	y += area_scale->get_h() + 8;
	add_subwindow(dust = new AgingToggle(plugin, x, y, &plugin->config.dust,
		_("Dust")));
	y += dust->get_h() + 8;
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

void AgingWindow::update()
{
	colorage->update(plugin->config.colorage);
	scratch->update(plugin->config.scratch);
	pits->update(plugin->config.pits);
	dust->update(plugin->config.dust);
	scratch_lines->update(plugin->config.scratch_lines);
	area_scale->update(plugin->config.area_scale);
}

AgingToggle::AgingToggle(AgingMain *plugin,
	int x,
	int y,
	int *output,
	const char *string)
 : BC_CheckBox(x, y, *output, string)
{
	this->plugin = plugin;
	this->output = output;
}

int AgingToggle::handle_event()
{
	*output = get_value();
	plugin->send_configure_change();
	return 1;
}


AgingSize::AgingSize(AgingMain *plugin, int x, int y,
	int *output, int min, int max)
 : BC_ISlider(x, y, 0, 200, 200, min, max, *output)
{
	this->plugin = plugin;
	this->output = output;
}

int AgingSize::handle_event()
{
	*output = get_value();
	plugin->send_configure_change();
	return 1;
}
