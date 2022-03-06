// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef TIPWINDOW_H
#define TIPWINDOW_H

#include "bcbutton.h"
#include "bcdialog.h"
#include "bctoggle.h"
#include "bcwindow.h"
#include "tipwindow.inc"

// Tip of the day to be run at initialization

class TipWindow : public BC_DialogThread
{
public:
	TipWindow();

	BC_Window* new_gui();
	char* get_current_tip();
	void next_tip();
	void prev_tip();

	TipWindowGUI *gui;
};


class TipWindowGUI : public BC_Window
{
public:
	TipWindowGUI(TipWindow *thread,
		int x,
		int y);

	int keypress_event();

	TipWindow *thread;
	BC_Title *tip_text;
};

class TipDisable : public BC_CheckBox
{
public:
	TipDisable(TipWindowGUI *gui, int x, int y);

	int handle_event();

	TipWindowGUI *gui;
};

class TipNext : public BC_Button
{
public:
	TipNext(TipWindowGUI *gui, int x, int y);

	int handle_event();
	static int calculate_w();

	TipWindowGUI *gui;
};

class TipPrev : public BC_Button
{
public:
	TipPrev(TipWindowGUI *gui, int x, int y);

	int handle_event();
	static int calculate_w();

	TipWindowGUI *gui;
};

class TipClose : public BC_Button
{
public:
	TipClose(TipWindowGUI *gui, int x, int y);

	int handle_event();

	static int calculate_w();
	static int calculate_h();

	TipWindowGUI *gui;
};

#endif
