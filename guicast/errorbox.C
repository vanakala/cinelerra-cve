
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

#include "errorbox.h"

ErrorBox::ErrorBox(const char *title, int x, int y, int w, int h)
 : BC_Window(title, x, y, w, h, w, h, 0)
{
}

ErrorBox::~ErrorBox()
{
}

int ErrorBox::create_objects(const char *text)
{
	int x = 10, y = 10;
	BC_Title *title;

	add_subwindow(new BC_Title(get_w() / 2, 
		y, 
		text, 
		MEDIUMFONT, 
		get_resources()->default_text_color, 
		1));
	x = get_w() / 2 - 30;
	y = get_h() - 50;
	add_tool(new BC_OKButton(x, y));
	return 0;
}
