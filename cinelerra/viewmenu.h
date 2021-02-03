// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef VIEWMENU_H
#define VIEWMENU_H

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
