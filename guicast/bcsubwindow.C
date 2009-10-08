
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

#include "bcsubwindow.h"



BC_SubWindow::BC_SubWindow(int x, int y, int w, int h, int bg_color)
 : BC_WindowBase()
{
	this->x = x; 
	this->y = y; 
	this->w = w; 
	this->h = h;
	this->bg_color = bg_color;
//printf("BC_SubWindow::BC_SubWindow 1\n");
}

BC_SubWindow::~BC_SubWindow()
{
}

int BC_SubWindow::initialize()
{
	create_window(parent_window, 
			"Sub Window", 
			x, 
			y, 
			w, 
			h, 
			0, 
			0, 
			0, 
			0, 
			0, 
			bg_color, 
			NULL, 
			SUB_WINDOW, 
			0,
			0);
	return 0;
}






BC_SubWindowList::BC_SubWindowList()
 : ArrayList<BC_WindowBase*>()
{
}

BC_SubWindowList::~BC_SubWindowList()
{
}
