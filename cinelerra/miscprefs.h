/*
 * CINELERRA
 * Copyright (C) 2016 Einar Rünkaru <einarrunkaru at gmail dot com>
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

#ifndef MISCPREFS_H
#define MISCPREFS_H

#include "mwindow.inc"
#include "preferencesthread.h"

class MiscPrefs : public PreferencesDialog
{
public:
	MiscPrefs(MWindow *mwindow, PreferencesWindow *pwindow);

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