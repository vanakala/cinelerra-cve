// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef LEVELWINDOWGUI_H
#define LEVELWINDOWGUI_H

class LevelWindowReset;

#include "bcwindow.h"
#include "levelwindow.inc"
#include "meterpanel.inc"

class LevelWindowGUI : public BC_Window
{
public:
	LevelWindowGUI();
	~LevelWindowGUI();

	void resize_event(int w, int h);
	void translation_event();
	void close_event();
	int keypress_event();

	MeterPanel *panel;
};

#endif
