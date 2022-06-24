// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bcwindow.h"

BC_Window::BC_Window(const char *title, int x,int y, int w, int h,
	int minw, int minh, int allow_resize,
	int private_color, int hide,
	int bg_color, const char *display_name,
	int group_it, int options)
 : BC_WindowBase()
{
	create_window(0, title, x, y, w, h,
		(minw < 0) ? w : minw, (minh < 0) ? h : minh,
		allow_resize, private_color, hide,
		bg_color, display_name, MAIN_WINDOW,
		0, group_it, options);
}
