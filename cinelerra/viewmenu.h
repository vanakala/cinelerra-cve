
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

#ifndef VIEWMENU_H
#define VIEWMENU_H

#include "guicast.h"
#include "mainmenu.inc"
#include "mwindow.inc"

class ShowAssets : public BC_MenuItem
{
public:
	ShowAssets(MWindow *mwindow, const char *hotkey);
	int handle_event();
	MWindow *mwindow;
};


class ShowTitles : public BC_MenuItem
{
public:
	ShowTitles(MWindow *mwindow, const char *hotkey);
	int handle_event();
	MWindow *mwindow;
};

class ShowTransitions : public BC_MenuItem
{
public:
	ShowTransitions(MWindow *mwindow, const char *hotkey);
	int handle_event();
	MWindow *mwindow;
};

class PluginAutomation : public BC_MenuItem
{
public:
	PluginAutomation(MWindow *mwindow, const char *hotkey);
	int handle_event();
	MWindow *mwindow;
};

class ShowAutomation : public BC_MenuItem
{
public:
	ShowAutomation(MWindow *mwindow, 
		const char *text,
		const char *hotkey,
		int subscript);
	int handle_event();
	void update_toggle();
	MWindow *mwindow;
	int subscript;
};


#endif
