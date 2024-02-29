// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef BCPOPUP_H
#define BCPOPUP_H

#include "arraylist.h"
#include "bcpixmap.inc"
#include "bcwindowbase.h"

class BC_Popup : public BC_WindowBase
{
public:
	BC_Popup(BC_WindowBase *parent_window,
		int x, int y, int w, int h,
		int bg_color, int hide = 0,
		BC_Pixmap *bg_pixmap = 0);
	virtual ~BC_Popup() {};

	void initialize() {};
};

class BC_FullScreen : public BC_WindowBase
{
public:
	BC_FullScreen(BC_WindowBase *parent_window, int w, int h,
		int bg_color, int vm_scale, int hide = 0,
		BC_Pixmap *bg_pixmap = 0);
	virtual ~BC_FullScreen() {};
};

#endif
