// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef MAINXSCROLL_H
#define MAINXSCROLL_H

#include "bcscrollbar.h"
#include "mutex.inc"

class SampleScroll : public BC_ScrollBar
{
public:
	SampleScroll(int x, int y, int w);
	~SampleScroll();

	void resize_event();
	void set_position();
	int handle_event();

private:
	Mutex *lock;
};

#endif
