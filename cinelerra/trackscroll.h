// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef TRACKSCROLL_H
#define TRACKSCROLL_H

#include "bcscrollbar.h"
#include "mwindow.inc"

class TrackScroll : public BC_ScrollBar
{
public:
	TrackScroll(MWindow *mwindow, int x, int y, int h);

	void resize_event();
	void set_position(int handle_length);
	int handle_event();

private:
	MWindow *mwindow;
};

#endif
