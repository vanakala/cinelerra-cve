// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2016 Einar Rünkaru <einarrunkaru at gmail dot com>

#include "bcsignals.h"
#include "bctitle.h"
#include "pluginclient.h"
#include "plugin.h"
#include "pluginwindow.h"

#include <stdarg.h>

PluginWindow::PluginWindow(PluginClient *pluginclient, int x, int y, int w, int h)
 : BC_Window(pluginclient->gui_string, x, y, w, h, w, h,
	0, 0, 1, -1, 0, 1, WINDOW_UTF8)
{
	window_done = 0;
	this->pluginclient = pluginclient;
};

void PluginWindow::close_event()
{
	if(window_done)
		return;
	window_done = 1;
	if(pluginclient && pluginclient->plugin)
	{
		pluginclient->plugin->show = 0;
		pluginclient->plugin->update_toggles();
	}
	BC_Window::close_event();
}

BC_Title *PluginWindow::print_title(int x, int y, const char *fmt, ...)
{
	BC_Title *new_title;
	va_list ap;
	char buffer[BCTEXTLEN];

	va_start(ap, fmt);
	vsprintf(buffer, fmt, ap);
	va_end(ap);

	return new BC_Title(x, y, buffer);
}
