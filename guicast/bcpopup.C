// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bcpopup.h"

BC_FullScreen::BC_FullScreen(BC_WindowBase *parent_window, int w, int h,
	int bg_color, int vm_scale, int hide, BC_Pixmap *bg_pixmap)
 : BC_WindowBase()
{
	if(parent_window)
		color_model = parent_window->color_model;
	else
		color_model = bg_color;

#ifdef HAVE_LIBXXF86VM
	if(vm_scale)
		create_window(parent_window, "Fullscreen",
			0, 0, w, h, w, h, 0,
			parent_window->top_level->private_color,
			hide, bg_color, NULL, VIDMODE_SCALED_WINDOW,
			bg_pixmap, 0);
	else
#endif
		create_window(parent_window, "Fullscreen",
			0, 0, w, h, w, h, 0,
			parent_window->top_level->private_color,
			hide, bg_color, NULL, POPUP_WINDOW,
			bg_pixmap, 0);
}


BC_Popup::BC_Popup(BC_WindowBase *parent_window,
	int x, int y, int w, int h,
	int bg_color, int hide, BC_Pixmap *bg_pixmap)
 : BC_WindowBase()
{
	if(parent_window)
		color_model = parent_window->color_model;
	else
		color_model = bg_color;

	create_window(parent_window, "Popup", x, y, w, h, w, h, 0,
		parent_window->top_level->private_color, hide,
		bg_color, NULL, POPUP_WINDOW, bg_pixmap, 0);
}

