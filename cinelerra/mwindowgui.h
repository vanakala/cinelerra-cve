// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef MAINWINDOWGUI_H
#define MAINWINDOWGUI_H

#include "bcwindow.h"
#include "colors.h"
#include "datatype.h"
#include "mbuttons.inc"
#include "mainclock.inc"
#include "maincursor.inc"
#include "mainmenu.inc"
#include "mtimebar.inc"
#include "patchbay.inc"
#include "zoombar.inc"
#include "samplescroll.inc"
#include "statusbar.inc"
#include "trackcanvas.inc"
#include "trackscroll.inc"


class MWindowGUI : public BC_Window
{
public:
	MWindowGUI();
	~MWindowGUI();

	void show();
	void get_scrollbars();

// ======================== event handlers

	void focus_out_event();

	void translation_event();
	void resize_event(int w, int h);          // handle a resize event
	int keypress_event();
	void close_event();
	void save_defaults(BC_Hash *defaults);
	void load_defaults(BC_Hash *defaults);
	int menu_h();

// Pop up a box if the statusbar is taken and show an error.
	void show_error(char *message, int color = BLACK);
	void repeat_event(int duration);
// Entry point for drag events in all windows
	void drag_motion();
	void drag_stop();

// Return if the area bounded by x1 and x2 is visible between view_x1 and view_x2
	static int visible(int x1, int x2, int view_x1, int view_x2);

	MainClock *mainclock;
	MButtons *mbuttons;
	MTimeBar *timebar;
	PatchBay *patchbay;
	MainMenu *mainmenu;
	ZoomBar *zoombar;
	SampleScroll *samplescroll;
	StatusBar *statusbar;
	TrackScroll *trackscroll;
	TrackCanvas *canvas;
// Cursor used to select regions
	MainCursor *cursor;
// Dimensions of canvas minus scrollbars
	int view_w, view_h;
};

#endif
