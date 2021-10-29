// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2016 Einar Rünkaru <einarrunkaru at gmail dot com>

#ifndef MISCPREFS_H
#define MISCPREFS_H

#include "preferencesthread.h"

class MiscPrefs : public PreferencesDialog
{
public:
	MiscPrefs(PreferencesWindow *pwindow);

	void show();
};


class StillImageUseDuration : public BC_CheckBox
{
public:
	StillImageUseDuration(PreferencesWindow *pwindow, int value, int x, int y);

	int handle_event();

	PreferencesWindow *pwindow;
};


class StillImageDuration : public BC_TextBox
{
public:
	StillImageDuration(PreferencesWindow *pwindow, int x, int y);

	int handle_event();

	PreferencesWindow *pwindow;
};

class ToggleButton : public BC_CheckBox
{
public:
	ToggleButton(int x, int y, const char *text, int *value);

	int handle_event();

	int *valueptr;
};

class MiscText : public BC_TextBox
{
public:
	MiscText(int x, int y, char *boxtext);

	int handle_event();
private:
	char *str;
};

class MiscValue : public BC_TextBox
{
public:
	MiscValue(int x, int y, int *value);

	int handle_event();
private:
	int *valueptr;
};

#endif
