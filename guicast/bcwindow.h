// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef BCWINDOW_H
#define BCWINDOW_H

#include "bcwindowbase.h"

class BC_Window : public BC_WindowBase
{
public:
	BC_Window(const char *title, int x, int y, int w, int h,
		int minw = -1, int minh = -1, int allow_resize = 1,
		int private_color = 0, int hide = 0,
		int bg_color = -1, const char *display_name = "",
		int group_it = 1, int options = 0);
	virtual ~BC_Window() {};
};

#endif
