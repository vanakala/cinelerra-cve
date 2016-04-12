
/*
 * Copyright (C) 2016 Einar Rünkaru <einarrunkaru at gmail dot com>
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

#include "bcsignals.h"
#include "pluginwindow.h"

PluginWindow::PluginWindow(const char *title, int x, int y, int w, int h)
 : BC_Window(title, x, y, w, h, w, h,
	0, 0, 1, -1, 0, 1, WINDOW_UTF8)
{
tracemsg("%p '%s'", this, title);
	window_done = 0;
};

void PluginWindow::close_event()
{
tracemsg("%p window_done %d", this, window_done);
	if(window_done)
		return;
	window_done = 1;
	BC_Window::close_event();
}

PluginWindow::~PluginWindow()
{
tracemsg("%p", this);
}
