// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef VTIMEBAR_H
#define VTIMEBAR_H

#include "timebar.h"
#include "vwindowgui.inc"

class VTimeBar : public TimeBar
{
public:
	VTimeBar(VWindowGUI *gui,
		int x,
		int y,
		int w, 
		int h);

	void resize_event();
	EDL* get_edl();
	void draw_time();
	void update_preview();
	void select_label(ptstime position);

	VWindowGUI *gui;
};

#endif
