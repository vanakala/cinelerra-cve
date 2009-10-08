
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

#include "cursor.h"

Cursor_::Cursor_(BC_SubWindow *canvas)
{
	this->canvas = canvas;
	selectionstart = selectionend = zoom_sample = viewstart = 0;
}

Cursor_::~Cursor_()
{
}

int Cursor_::show(int flash, long selectionstart, long selectionend, long zoom_sample, long viewstart, int vertical)
{
return 0;
	this->selectionstart = selectionstart;
	this->selectionend = selectionend;
	this->viewstart = viewstart;
	this->zoom_sample = zoom_sample;
	this->vertical = vertical;
	draw(flash, selectionstart, selectionend, zoom_sample, viewstart, vertical);
}

int Cursor_::hide(int flash)
{
return 0;
	draw(flash, selectionstart, selectionend, zoom_sample, viewstart, vertical);
}

int Cursor_::draw(int flash, long selectionstart, long selectionend, long zoom_sample, long viewstart, int vertical)
{
return 0;
	if(canvas->get_h() * canvas->get_w() == 0) return 1; 
	if(zoom_sample == 0) return 1;       // no canvas

	long start = viewstart;
	long end = start + (long)(vertical ? canvas->get_h() : canvas->get_w()) * zoom_sample;
	int pixel1, pixel2;

	if((selectionstart >= start && selectionstart <= end) ||
		 (selectionend >= start && selectionend <= end) ||
		 (start >= selectionstart && end <= selectionend))
	{
		if(selectionstart < start)
		{
			pixel1 = 0;
		}
		else
		{
			pixel1 = (selectionstart - start) / zoom_sample;
		}
		
		if(selectionend > end)
		{
			pixel2 = (vertical ? canvas->get_h() : canvas->get_w());
		}
		else
		{
			pixel2 = (selectionend - start) / zoom_sample;
		}
		pixel2++;

		canvas->set_inverse();
		canvas->set_color(WHITE);
		
		if(vertical)
		canvas->draw_box(0, pixel1, canvas->get_w(), pixel2 - pixel1);
		else
		canvas->draw_box(pixel1, 0, pixel2 - pixel1, canvas->get_h());
		
		
		canvas->set_opaque();
	}
	if(flash) canvas->flash();
}

int Cursor_::resize(int w, int h)
{
}
