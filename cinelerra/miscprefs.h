// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2016 Einar Rünkaru <einarrunkaru at gmail dot com>

#ifndef MISCPREFS_H
#define MISCPREFS_H

#include "guielements.h"
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

#endif
