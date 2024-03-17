// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef LOADMODE_H
#define LOADMODE_H

#include "loadmode.inc"
#include "mwindow.inc"
#include "selection.h"

class LoadMode
{
public:
	LoadMode(BC_WindowBase *window,
		int x, int y, int *output, int use_nothing);

	friend class InsertionModeSelection;

	void reposition_window(int x, int y);
	static const char* name(int mode);
private:
	static const struct selection_int insertion_modes[];
	BC_Title *title;
	InsertionModeSelection *modeselection;
};

class InsertionModeSelection : public Selection
{
public:
	InsertionModeSelection(int x, int y,
		BC_WindowBase *base, int *value, int optmask);

	void update(int value);
};

#endif
