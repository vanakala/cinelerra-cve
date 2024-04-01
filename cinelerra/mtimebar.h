// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef MTIMEBAR_H
#define MTIMEBAR_H

#include "mwindowgui.inc"
#include "timebar.h"

class MTimeBar : public TimeBar
{
public:
	MTimeBar(MWindowGUI *gui, int x, int y, int w, int h);

	void draw_time();
	void draw_range();
	void stop_playback();
	void resize_event();
	int test_preview(int buttonpress);
	int position_to_pixel(ptstime position);
	void select_label(double position);

	MWindowGUI *gui;
};

#endif
