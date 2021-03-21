// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef MAINCURSOR_H
#define MAINCURSOR_H

#include "datatype.h"
#include "mwindowgui.inc"
#include "mutex.h"

class MainCursor
{
public:
	MainCursor(MWindowGUI *gui);

	void repeat_event(int duration);
	void hide();
	void activate();
	void deactivate();
	void invisible();
	void show();
	void update();
	void focus_out_event();

	int playing_back;

private:
	void draw();
	void flash();

	MWindowGUI *gui;
	int pixel1;
	int pixel2;
	int visible;
	int active;
	int enabled;
	Mutex cursor_lock;
};

#endif
