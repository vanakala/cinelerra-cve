// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2013 Einar Rünkaru <einarrunkaru@gmail dot com>

#ifndef RULER_H
#define RULER_H

#include "bcwindow.h"
#include "ruler.inc"
#include "thread.h"

class RulerGUI;

class Ruler : public Thread
{
public:
	Ruler();
	~Ruler();

	void run();

	RulerGUI *gui;
};

class RulerGUI : public BC_Window
{
public:
	RulerGUI(Ruler *thread);

	void draw_ruler();
	void resize_event(int w, int h);
	void translation_event();
	void close_event();
	int keypress_event();
	int button_release_event();
	int drag_start_event();
	int cursor_motion_event();
	void drag_motion_event();
	void drag_stop_event();

private:
	int ticklength(int tick);

	int resize;
	int width;
	int height;
	// center of closing button
	int x_cl;
	int y_cl;

	int max_length;
	int min_length;
	int root_w;
	int root_h;
	int separate_cursor;
};

#endif
