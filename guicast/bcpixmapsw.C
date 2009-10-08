
/*
 * CINELERRA
 * Copyright (C) 2005 Pierre Dumuid
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

#include "bcpixmap.h"
#include "bcpixmapsw.h"
#include "bcresources.h"
#include "colors.h"
#include "keys.h"
#include "units.h"
#include "vframe.h"

#include <ctype.h>
#include <math.h>
#include <string.h>

BC_PixmapSW::BC_PixmapSW(int x, int y, BC_Pixmap *thepixmap)
 : BC_SubWindow(x, y, -1, -1, -1)
{
	this->thepixmap = thepixmap;
}

BC_PixmapSW::~BC_PixmapSW()
{
}

int BC_PixmapSW::initialize()
{
	w = thepixmap->get_w();
	h = thepixmap->get_h();

	BC_SubWindow::initialize();
	draw();
	return 0;
}

int BC_PixmapSW::reposition_widget(int x, int y)
{
	BC_WindowBase::reposition_window(x, y);
	draw();
	return 0;
}

int BC_PixmapSW::draw()
{
	draw_top_background(parent_window, 0, 0, get_w(), get_h());
	draw_pixmap(thepixmap);
	flash();
	return 0;
}

