
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

#ifndef CURSOR_H
#define CURSOR_H

#include "guicast.h"

class Cursor_
{
public:
	Cursor_(BC_SubWindow *canvas);
	~Cursor_();

	int resize(int w, int h);
	int show(int flash, long selectionstart, long selectionend, long zoom_sample, long viewstart, int vertical);
	int hide(int flash);
	int draw(int flash, long selectionstart, long selectionend, long zoom_sample, long viewstart, int vertical);

	BC_SubWindow *canvas;
	int vertical;
	long selectionstart, selectionend, zoom_sample, viewstart;
};

#endif
