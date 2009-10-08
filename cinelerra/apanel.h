
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

#ifndef APANEL_H
#define APANEL_H


#include "cwindowgui.inc"
#include "guicast.h"
#include "mwindow.inc"
#include "track.inc"



class APanelPluginAuto;
class APanelMute;
class APanelPlay;

class APanel
{
public:
	APanel(MWindow *mwindow, CWindowGUI *subwindow, int x, int y, int w, int h);
	~APanel();

	int create_objects();

	MWindow *mwindow;
	CWindowGUI *subwindow;
	int x, y, w, h;
	APanelPluginAuto *plugin_autos[PLUGINS];
	APanelMute *mute;
	APanelPlay *play;
};

class APanelPluginAuto : public BC_FPot
{
public:
	APanelPluginAuto(MWindow *mwindow, APanel *gui, int x, int y);
	int handle_event();
	MWindow *mwindow;
	APanel *gui;
};


class APanelMute : public BC_CheckBox
{
public:
	APanelMute(MWindow *mwindow, APanel *gui, int x, int y);
	int handle_event();
	MWindow *mwindow;
	APanel *gui;
};


class APanelPlay : public BC_CheckBox
{
public:
	APanelPlay(MWindow *mwindow, APanel *gui, int x, int y);
	int handle_event();
	MWindow *mwindow;
	APanel *gui;
};


#endif
