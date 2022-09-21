// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef INTERFACEPREFS_H
#define INTERFACEPREFS_H

#include "bcpopupmenu.h"
#include "browsebutton.h"
#include "deleteallindexes.inc"
#include "preferencesthread.h"
#include "selection.h"


class InterfacePrefs : public PreferencesDialog
{
public:
	InterfacePrefs(PreferencesWindow *pwindow);

	void show();
private:
	BrowseButton *ipath;
	DeleteAllIndexes *deleteall;

	static const struct selection_int time_formats[];
};


class ViewBehaviourSelection : public Selection
{
public:
	ViewBehaviourSelection(int x, int y, BC_WindowBase *window,
		int *value);

	void update(int value);
	static const char *name(int value);
private:
	static const struct selection_int viewbehaviour[];
};


class ViewTheme : public BC_PopupMenu
{
public:
	ViewTheme(int x, int y, PreferencesWindow *pwindow);

	PreferencesWindow *pwindow;
};


class ViewThemeItem : public BC_MenuItem
{
public:
	ViewThemeItem(ViewTheme *popup, const char *text);

	int handle_event();

	ViewTheme *popup;
};

#endif
