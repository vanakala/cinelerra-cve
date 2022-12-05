// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2007 Andraz Tori

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
	RenderProfile(RenderWindow *rwindow, int x, int y);
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
