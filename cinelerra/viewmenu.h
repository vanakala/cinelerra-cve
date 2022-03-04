// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef VIEWMENU_H
#define VIEWMENU_H

#include "mainmenu.inc"

class ShowAssets : public BC_MenuItem
{
public:
	ShowAssets(const char *hotkey);

	int handle_event();
};


class ShowTitles : public BC_MenuItem
{
public:
	ShowTitles(const char *hotkey);

	int handle_event();
};

class ShowTransitions : public BC_MenuItem
{
public:
	ShowTransitions(const char *hotkey);

	int handle_event();
};

class PluginAutomation : public BC_MenuItem
{
public:
	PluginAutomation(const char *hotkey);

	int handle_event();
};

class ShowAutomation : public BC_MenuItem
{
public:
	ShowAutomation(const char *text,
		const char *hotkey,
		int subscript);

	int handle_event();
	void update_toggle();

	int subscript;
};


#endif
