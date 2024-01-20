// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef BCSUBWINDOW_H
#define BCSUBWINDOW_H

#include "arraylist.h"
#include "bcwindowbase.h"

class BC_SubWindow : public BC_WindowBase
{
public:
	BC_SubWindow(int x, int y, int w, int h, int bg_color = -1);
	virtual ~BC_SubWindow() {};

	virtual void initialize();
private:
};

class BC_SubWindowList : public ArrayList<BC_WindowBase*>
{
public:
	BC_SubWindowList();

};

#endif
