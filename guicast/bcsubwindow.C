// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bcsubwindow.h"

BC_SubWindow::BC_SubWindow(int x, int y, int w, int h, int bg_color)
 : BC_WindowBase()
{
	this->x = x;
	this->y = y;
	this->w = w;
	this->h = h;
	this->bg_color = bg_color;
}

void BC_SubWindow::initialize()
{
	if(parent_window)
		color_model = parent_window->color_model;
	else
		color_model = bg_color;

	create_window(parent_window, "Sub Window",
		x, y, w, h, 0, 0, 0, 0, 0, bg_color, NULL, SUB_WINDOW, 0, 0);
}


BC_SubWindowList::BC_SubWindowList()
 : ArrayList<BC_WindowBase*>()
{
}
