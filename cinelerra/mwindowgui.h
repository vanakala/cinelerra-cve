
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */

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
#include "mwindow.inc"
#include "patchbay.inc"
#include "zoombar.inc"
#include "samplescroll.inc"
#include "statusbar.inc"
#include "trackcanvas.inc"
#include "trackscroll.inc"


class MWindowGUI : public BC_Window
{
public:
	MWindowGUI(MWindow *mwindow);
	~MWindowGUI();

	void show();
	void get_scrollbars();

// ======================== event handlers

// Replace with update
	void redraw_time_dependancies();
	void focus_out_event();

// options (bits):
//          WUPD_SCROLLBARS - update scrollbars
//          WUPD_CANVINCR   - incremental drawing of resources
//          WUPD_CANVREDRAW - delete and redraw of resources
//          WUPD_CANVPICIGN -  ignore picon thread
//          WUPD_TIMEBAR    - update timebar
//          WUPD_ZOOMBAR    - update zoombar
//          WUPD_PATCHBAY   - update patchbay
//          WUPD_CLOCK      - update clock
//          WUPD_BUTTONBAR  - update buttonbar
	void update(int options);
	void translation_event();
	void resize_event(int w, int h);          // handle a resize event
	int keypress_event();
	void close_event();
	void save_defaults(BC_Hash *defaults);
	int menu_h();

// Pop up a box if the statusbar is taken and show an error.
	void show_error(char *message, int color = BLACK);
	void repeat_event(int duration);
// Entry point for drag events in all windows
	void drag_motion();
	int drag_stop();
	void default_positions();

// Return if the area bounded by x1 and x2 is visible between view_x1 and view_x2
	static int visible(int x1, int x2, int view_x1, int view_x2);

	MWindow *mwindow;

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
