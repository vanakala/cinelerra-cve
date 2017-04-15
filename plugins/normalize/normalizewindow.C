
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
#include "mwindow.inc"
#include "normalizewindow.h"


NormalizeWindow::NormalizeWindow(NormalizeMain *plugin, int x, int y)
 : PluginWindow(plugin->gui_string,
	x - 160,
	y - 75,
	320,
	150)
{
	x = y = 10;
	add_subwindow(new BC_Title(x, y, _("Enter the DB to overload by:")));
	y += 20;
	add_subwindow(new NormalizeWindowOverload(plugin, x, y));
	y += 30;
	add_subwindow(new NormalizeWindowSeparate(plugin, x, y));
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

NormalizeWindow::~NormalizeWindow()
{
}


NormalizeWindowOverload::NormalizeWindowOverload(NormalizeMain *plugin, int x, int y)
 : BC_TextBox(x, y, 200, 1, plugin->db_over)
{
	this->plugin = plugin;
}

NormalizeWindowOverload::~NormalizeWindowOverload()
{
}

int NormalizeWindowOverload::handle_event()
{
	plugin->db_over = atof(get_text());
	return 1;
}


NormalizeWindowSeparate::NormalizeWindowSeparate(NormalizeMain *plugin, int x, int y)
 : BC_CheckBox(x, y, plugin->separate_tracks, _("Treat tracks independantly"))
{
	this->plugin = plugin;
}

NormalizeWindowSeparate::~NormalizeWindowSeparate()
{
}

int NormalizeWindowSeparate::handle_event()
{
	plugin->separate_tracks = get_value();
	return 1;
}
