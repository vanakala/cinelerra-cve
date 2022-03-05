// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef CTIMEBAR_H
#define CTIMEBAR_H

#include "cwindowgui.inc"
#include "timebar.h"


class CTimeBar : public TimeBar
{
public:
	CTimeBar(CWindowGUI *gui,
		int x,
		int y,
		int w, 
		int h);

	void resize_event();
	void draw_time();
	void update_preview();
	void select_label(ptstime position);

	CWindowGUI *gui;
};

#endif
