
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

#include "bctitle.h"
#include "language.h"
#include "holowindow.h"


PLUGIN_THREAD_METHODS


HoloWindow::HoloWindow(HoloMain *plugin, int x, int y)
 : PluginWindow(plugin->gui_string, 
	x,
	y,
	300, 
	170)
{
	x = 10;
	y = 10;

	add_subwindow(new BC_Title(x, y, 
		_("HolographicTV from EffectTV\n"
		"Copyright (C) 2001 FUKUCHI Kentarou")
	));
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

HoloWindow::~HoloWindow()
{
}

void HoloWindow::update()
{
}
