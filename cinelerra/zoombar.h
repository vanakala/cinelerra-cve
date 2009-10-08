
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

#ifndef ZOOMBAR_H
#define ZOOMBAR_H

class FromTextBox;
class LengthTextBox;
class ToTextBox;


class SampleZoomPanel;
class AmpZoomPanel;
class TrackZoomPanel;
class AutoZoom;
class AutoTypeMenu;
class ZoomTextBox;

#include "guicast.h"
#include "mwindow.inc"
#include "mwindowgui.inc"
#include "zoompanel.h"

class ZoomBar : public BC_SubWindow
{
public:
	ZoomBar(MWindow *mwindow, MWindowGUI *gui);
	~ZoomBar();

	int create_objects();
	void resize_event();
	int draw();
	int resize_event(int w, int h);
	void redraw_time_dependancies();
	int update();          // redraw the current values
	void update_autozoom();
	int update_clocks();
	int update_playback(int64_t new_position);       // update the playback position
	int set_selection(int which_one);
	void update_formatting(BC_TextBox *dst);

	MWindow *mwindow;
	MWindowGUI *gui;
	SampleZoomPanel *sample_zoom;
	AmpZoomPanel *amp_zoom;
	TrackZoomPanel *track_zoom;
	AutoZoom *auto_zoom;
	AutoTypeMenu *auto_type;
	ZoomTextBox *auto_zoom_text;

	BC_Title *zoom_value, *playback_value;
	LengthTextBox *length_value;
	FromTextBox *from_value;
	ToTextBox *to_value;
	char string[256], string2[256];
	int64_t old_position;
};

class SampleZoomPanel : public ZoomPanel
{
public:
	SampleZoomPanel(MWindow *mwindow, ZoomBar *zoombar, int x, int y);
	int handle_event();
	MWindow *mwindow;
	ZoomBar *zoombar;
};

class AmpZoomPanel : public ZoomPanel
{
public:
	AmpZoomPanel(MWindow *mwindow, ZoomBar *zoombar, int x, int y);
	int handle_event();
	MWindow *mwindow;
	ZoomBar *zoombar;
};

class TrackZoomPanel : public ZoomPanel
{
public:
	TrackZoomPanel(MWindow *mwindow, ZoomBar *zoombar, int x, int y);
	int handle_event();
	MWindow *mwindow;
	ZoomBar *zoombar;
};

class AutoZoom : public BC_Tumbler
{
public:
	AutoZoom(MWindow *mwindow, ZoomBar *zoombar, int x, int y, int changemax);
	int handle_up_event();
	int handle_down_event();
	MWindow *mwindow;
	ZoomBar *zoombar;
	int changemax;
};


class AutoTypeMenu : public BC_PopupMenu
{
public:
	AutoTypeMenu(MWindow *mwindow,
		     ZoomBar *zoombar,
		     int x, 
		     int y);
	void create_objects();
	static char* to_text(int shape);
	static int from_text(char *text);
	int handle_event();
	MWindow *mwindow;
	ZoomBar *zoombar;
};


class ZoomTextBox : public BC_TextBox
{
public:
	ZoomTextBox(MWindow *mwindow, ZoomBar *zoombar, int x, int y, char *text);
	int button_press_event();
	int handle_event();
	MWindow *mwindow;
	ZoomBar *zoombar;
};








class FromTextBox : public BC_TextBox
{
public:
	FromTextBox(MWindow *mwindow, ZoomBar *zoombar, int x, int y);
	int handle_event();
	int update_position(double new_position);
	char string[256], string2[256];
	MWindow *mwindow;
	ZoomBar *zoombar;
};


class LengthTextBox : public BC_TextBox
{
public:
	LengthTextBox(MWindow *mwindow, ZoomBar *zoombar, int x, int y);
	int handle_event();
	int update_position(double new_position);
	char string[256], string2[256];
	MWindow *mwindow;
	ZoomBar *zoombar;
};

class ToTextBox : public BC_TextBox
{
public:
	ToTextBox(MWindow *mwindow, ZoomBar *zoombar, int x, int y);
	int handle_event();
	int update_position(double new_position);
	char string[256], string2[256];
	MWindow *mwindow;
	ZoomBar *zoombar;
};




#endif
