
/*
 * CINELERRA
 * Copyright (C) 2007 Andraz Tori
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

#ifndef RENDERPROFILE_H
#define RENDERPROFILE_H

#include "bcbutton.h"
#include "bclistbox.h"
#include "bctextbox.inc"
#include "bctitle.inc"
#include "render.inc"
#include "mwindow.inc"
#include "renderprofiles.inc"

class RenderProfileListBox;

class SaveRenderProfileButton : public BC_GenericButton
{
public:
	SaveRenderProfileButton(RenderProfile *profile, int x, int y);

	int handle_event();
	RenderProfile *profile;
};

class DeleteRenderProfileButton : public BC_GenericButton
{
public:
	DeleteRenderProfileButton(RenderProfile *profile, int x, int y);

	int handle_event();
	RenderProfile *profile;
};


class RenderProfile
{
public:
	RenderProfile(MWindow *mwindow,
		RenderWindow *rwindow, 
		int x, 
		int y);
	~RenderProfile();

	int create_profile(const char *profile);
	// create directory if it does not exist
	static int chk_profile_dir(const char *dirname);
	static void remove_profiledir(const char *dirname);
	int select_profile(const char *profile);
	void merge_profile(const char *profile);
	void reposition_window(int x, int y);
	static int calculate_h(BC_WindowBase *gui);
	int get_h();
	int get_x();
	int get_y();

	BC_Title *title;
	BC_TextBox *textbox;
	RenderProfileListBox *listbox;
	SaveRenderProfileButton *saveprofile;
	DeleteRenderProfileButton *deleteprofile;
	MWindow *mwindow;
	RenderWindow *rwindow;
	int x;
	int y;
	int *output;
	int use_nothing;
	ArrayList<BC_ListBoxItem*> profiles;
};

class RenderProfileListBox : public BC_ListBox
{
public:
	RenderProfileListBox(BC_WindowBase *window, RenderProfile *renderprofile, int x, int y);

	int handle_event();

	BC_WindowBase *window;
	RenderProfile *renderprofile;
};

#endif
