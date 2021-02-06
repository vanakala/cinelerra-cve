// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef LEVELWINDOW_H
#define LEVELWINDOW_H

#include "levelwindowgui.inc"
#include "thread.h"


class LevelWindow : public Thread
{
public:
	LevelWindow();
	~LevelWindow();

	void run();

	LevelWindowGUI *gui;
};

#endif
