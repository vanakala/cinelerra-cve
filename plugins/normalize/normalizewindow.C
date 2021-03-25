// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bctitle.h"
#include "language.h"
#include "normalizewindow.h"


NormalizeWindow::NormalizeWindow(NormalizeMain *plugin, int x, int y)
 : PluginWindow(plugin,
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


NormalizeWindowOverload::NormalizeWindowOverload(NormalizeMain *plugin, int x, int y)
 : BC_TextBox(x, y, 200, 1, (float)plugin->db_over)
{
	this->plugin = plugin;
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

int NormalizeWindowSeparate::handle_event()
{
	plugin->separate_tracks = get_value();
	return 1;
}
