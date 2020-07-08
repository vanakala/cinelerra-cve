// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

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

void HoloWindow::update()
{
}
