// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef GWINDOW_H
#define GWINDOW_H

#include "gwindowgui.inc"
#include "bcwindowbase.h"
#include "thread.h"

class GWindow : public Thread
{
public:
	GWindow(BC_WindowBase *win);

	void run();

	GWindowGUI *gui;
};

#endif
