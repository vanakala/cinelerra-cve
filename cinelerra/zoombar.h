// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

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

#include "bcsubwindow.h"
#include "bctextbox.h"
#include "zoompanel.h"


class ZoomBar : public BC_SubWindow
{
public:
	ZoomBar();
	~ZoomBar();

	void show();
	void resize_event();
	void draw();
	void resize_event(int w, int h);
	void redraw_time_dependancies();
	void update();          // redraw the current values
	void update_autozoom();
	void update_clocks();
	void set_selection(int which_one);
	void update_formatting(BC_TextBox *dst);

	SampleZoomPanel *sample_zoom;
	AmpZoomPanel *amp_zoom;
	TrackZoomPanel *track_zoom;
	AutoZoom *auto_zoom;
	AutoTypeMenu *auto_type;
	ZoomTextBox *auto_zoom_text;

	LengthTextBox *length_value;
	FromTextBox *from_value;
	ToTextBox *to_value;
	char string[256], string2[256];
};


class SampleZoomPanel : public ZoomPanel
{
public:
	SampleZoomPanel(ZoomBar *zoombar, int x, int y);

	int handle_event();

	ZoomBar *zoombar;
};


class AmpZoomPanel : public ZoomPanel
{
public:
	AmpZoomPanel(ZoomBar *zoombar, int x, int y);

	int handle_event();

	ZoomBar *zoombar;
};


class TrackZoomPanel : public ZoomPanel
{
public:
	TrackZoomPanel(ZoomBar *zoombar, int x, int y);

	int handle_event();

	ZoomBar *zoombar;
};


class AutoZoom : public BC_Tumbler
{
public:
	AutoZoom(ZoomBar *zoombar, int x, int y, int changemax);

	void handle_up_event();
	void handle_down_event();

	ZoomBar *zoombar;
	int changemax;
};


class AutoTypeMenu : public BC_PopupMenu
{
public:
	AutoTypeMenu(ZoomBar *zoombar, int x, int y);

	static const char* to_text(int shape);
	static int from_text(const char *text);
	int handle_event();

	ZoomBar *zoombar;
};


class ZoomTextBox : public BC_TextBox
{
public:
	ZoomTextBox(ZoomBar *zoombar, int x, int y, const char *text);

	int button_press_event();
	int handle_event();

	ZoomBar *zoombar;
};


class FromTextBox : public BC_TextBox
{
public:
	FromTextBox(ZoomBar *zoombar, int x, int y);

	int handle_event();
	void update_position(ptstime new_position);

	ZoomBar *zoombar;
};


class LengthTextBox : public BC_TextBox
{
public:
	LengthTextBox(ZoomBar *zoombar, int x, int y);

	int handle_event();
	void update_position(ptstime new_position);

	ZoomBar *zoombar;
};

class ToTextBox : public BC_TextBox
{
public:
	ToTextBox(ZoomBar *zoombar, int x, int y);

	int handle_event();
	void update_position(ptstime new_position);

	ZoomBar *zoombar;
};

#endif
