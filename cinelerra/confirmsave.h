// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef CONFIRMSAVE_H
#define CONFIRMSAVE_H

#include "arraylist.h"
#include "asset.inc"
#include "bclistboxitem.h"
#include "bcwindow.h"
#include "mwindow.inc"

class ConfirmSaveOkButton;
class ConfirmSaveCancelButton;

class ConfirmSave
{
public:
	ConfirmSave();

// Return values:
// 1 cancel
// 0 replace or doesn't exist yet
	static int test_file(const char *path);
	static int test_files(ArrayList<char*> *paths);

};

class ConfirmSaveWindow : public BC_Window
{
public:
	ConfirmSaveWindow(ArrayList<BC_ListBoxItem*> *list,
		int absx, int absy);

	void resize_event(int w, int h);

	ArrayList<BC_ListBoxItem*> *list;
	BC_Title *title;
	BC_ListBox *listbox;
};

#endif
