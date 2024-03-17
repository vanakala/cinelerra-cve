// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef SPLASHGUI_H
#define SPLASHGUI_H

#include "bctitle.h"
#include "bcprogress.inc"
#include "bcwindow.h"
#include "vframe.inc"


class SplashGUI : public BC_Window
{
public:
	SplashGUI(VFrame *bg, int x, int y);

	BC_Title *operation;
	BC_ProgressBar *progress;
};

#endif
