// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef STATUSBAR_H
#define STATUSBAR_H

#include "bcbutton.h"
#include "bcsubwindow.h"

class StatusBarCancel;

class StatusBar : public BC_SubWindow
{
public:
	StatusBar();

	void show();
	void resize_event();

	BC_ProgressBar *main_progress;
	StatusBarCancel *main_progress_cancel;
	BC_Title *status_text;
};

class StatusBarCancel : public BC_Button
{
public:
	StatusBarCancel(int x, int y);

	int handle_event();
};

#endif
