
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

#include "editpopup.inc"
#include "guicast.h"
#include "keyframepopup.inc"
#include "mbuttons.inc"
#include "mainclock.inc"
#include "maincursor.inc"
#include "mainmenu.inc"
#include "mtimebar.inc"
#include "mwindow.inc"
#include "patchbay.inc"
#include "pluginpopup.inc"
#include "zoombar.inc"
#include "samplescroll.inc"
#include "statusbar.inc"
#include "trackcanvas.inc"
#include "trackscroll.inc"
#include "transitionpopup.inc"


class MWindowGUI : public BC_Window
{
public:
	MWindowGUI(MWindow *mwindow);
	~MWindowGUI();

	int create_objects();
	void get_scrollbars();

// ======================== event handlers

// Replace with update
	void redraw_time_dependancies();

	int focus_in_event();
	int focus_out_event();

// canvas - 1 for incremental drawing of resources
//          2 for delete and redraw of resources
//          3 to ignore picon thread
	void update(int scrollbars,
		int canvas, 
		int timebar,
		int zoombar,
		int patchbay, 
		int clock,
		int buttonbar);

	void update_title(char *path);
	int translation_event();
	int resize_event(int w, int h);          // handle a resize event
	int keypress_event();
	int close_event();
	int quit();
	int save_defaults(BC_Hash *defaults);
	int menu_h();
// Draw on the status bar only.
	int show_message(char *message, int color = -1);
// Pop up a box if the statusbar is taken and show an error.
	void show_error(char *message, int color = BLACK);
	int repeat_event(int64_t duration);
// Entry point for drag events in all windows
	int drag_motion();
	int drag_stop();
	void default_positions();

// Return if the area bounded by x1 and x2 is visible between view_x1 and view_x2
	static int visible(int64_t x1, int64_t x2, int64_t view_x1, int64_t view_x2);

	MWindow *mwindow;

// Popup menus
	EditPopup *edit_menu;
	PluginPopup *plugin_menu;
	KeyframePopup *keyframe_menu;
	TransitionPopup *transition_menu;

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
