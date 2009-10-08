
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

#ifndef MICROTHEME_H
#define MICROTHEME_H

#include "plugintclient.h"
#include "statusbar.inc"
#include "theme.h"
#include "trackcanvas.inc"
#include "timebar.inc"

class MicroTheme : public Theme
{
public:
	MicroTheme();
	~MicroTheme();

	void draw_mwindow_bg(MWindowGUI *gui);
	void get_mwindow_sizes(MWindowGUI *gui, int w, int h);
	void get_cwindow_sizes(CWindowGUI *gui);
	void get_vwindow_sizes(VWindowGUI *gui);
	void get_recordgui_sizes(RecordGUI *gui, int w, int h);

	void initialize();
};

class MicroThemeMain : public PluginTClient
{
public:
	MicroThemeMain(PluginServer *server);
	~MicroThemeMain();

	char* plugin_title();
	Theme* new_theme();
	
	MicroTheme *theme;
};




#endif
