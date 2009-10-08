
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

#ifndef CONFIRMSAVE_H
#define CONFIRMSAVE_H

#include "guicast.h"
#include "mwindow.inc"

class ConfirmQuitYesButton;
class ConfirmQuitNoButton;
class ConfirmQuitCancelButton;

class ConfirmQuitWindow : public BC_Window
{
public:
	ConfirmQuitWindow(MWindow *mwindow);
	~ConfirmQuitWindow();

	int create_objects(char *string);

	MWindow *mwindow;
};

class ConfirmQuitYesButton : public BC_GenericButton
{
public:
	ConfirmQuitYesButton(MWindow *mwindow, ConfirmQuitWindow *gui);

	int handle_event();
	int keypress_event();
};

class ConfirmQuitNoButton : public BC_GenericButton
{
public:
	ConfirmQuitNoButton(MWindow *mwindow, ConfirmQuitWindow *gui);

	int handle_event();
	int keypress_event();
};

class ConfirmQuitCancelButton : public BC_GenericButton
{
public:
	ConfirmQuitCancelButton(MWindow *mwindow, ConfirmQuitWindow *gui);

	int handle_event();
	int keypress_event();
};

#endif
