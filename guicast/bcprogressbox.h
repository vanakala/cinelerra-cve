// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef BCPROGRESSBOX_H
#define BCPROGRESSBOX_H

#include "bcprogress.inc"
#include "bcprogressbox.inc"
#include "bctitle.inc"
#include "bcwindow.h"
#include "thread.h"

class BC_ProgressBox : public Thread
{
public:
	BC_ProgressBox(int x, int y, const char *text, int64_t length);
	virtual ~BC_ProgressBox();

	friend class BC_ProgressWindow;

	void run();
	int update(double position, int lock_it);    // return 1 if cancelled
	int update_title(const char *title, int lock_it);
	int update_length(double length, int lock_it);
	int is_cancelled();      // return 1 if cancelled
	void stop_progress();

private:
	BC_ProgressWindow *pwindow;
	char *display;
	char *text;
	int cancelled;
	double length;
};


class BC_ProgressWindow : public BC_Window
{
public:
	BC_ProgressWindow(int x, int y, const char *text, double length);

	const char *text;
	BC_ProgressBar *bar;
	BC_Title *caption;
};

#endif
