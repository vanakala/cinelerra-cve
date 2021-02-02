// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2004 Andraz Tori

#ifndef MANUALGOTO_H
#define MANUALGOTO_H

#include "bctextbox.h"
#include "bcwindow.h"
#include "datatype.h"
#include "mwindow.inc"
#include "thread.h"
#include "vwindow.inc"

class ManualGotoWindow;
class ManualGotoNumber;

#define NUM_TIMEPARTS 5

class ManualGoto : public Thread
{
public:
	ManualGoto(MWindow *mwindow, BC_WindowBase *masterwindow);

	void run();

// If it is being created or edited
	MWindow *mwindow;
	BC_WindowBase *masterwindow;
	void open_window();

	ptstime position;
	VFrame *icon_image;
	ManualGotoWindow *window;
};


class ManualGotoWindow : public BC_Window
{
public:
	ManualGotoWindow(MWindow *mwindow, ManualGoto *thread, int absx, int absy);

	void activate();
	ptstime get_entered_position_sec();
	void set_entered_position_sec(ptstime position);
	int split_timestr(char *timestr);

// Use this copy of the pointer in ManualGoto since multiple windows are possible
	BC_Title *signtitle;
	ManualGotoNumber *boxes[NUM_TIMEPARTS];
	int numboxes;
	MWindow *mwindow;
	ManualGoto *thread;
	int timeformat;
	char timestring[64];
	char *timeparts[NUM_TIMEPARTS];
};


class ManualGotoNumber : public BC_TextBox
{
public:
	ManualGotoNumber(ManualGotoWindow *window, int x, int y, int w, int chars);

	int handle_event();
	int keypress_event();
	void activate();
	void deactivate();
	void reshape_update(char *nums);

	ManualGotoWindow *window;
	int min_num;
	int max_num;
	int chars;
};
#endif
