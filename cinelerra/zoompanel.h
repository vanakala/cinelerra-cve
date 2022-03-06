// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef ZOOMPANEL_H
#define ZOOMPANEL_H

#include "bcpopupmenu.h"
#include "bctumble.h"
#include "units.h"
#include "vframe.inc"

class ZoomPopup;
class ZoomTumbler;

#define ZOOM_PERCENTAGE 0
#define ZOOM_FLOAT 1
#define ZOOM_TIME 2
#define ZOOM_LONG 3

class ZoomHash
{
public:
	ZoomHash(double value, const char *text);
	~ZoomHash();

	double value;
	char *text;
} ;

class ZoomPanel
{
public:
	ZoomPanel(BC_WindowBase *subwindow,
		double value, 
		int x, 
		int y, 
		int w, 
		double min = 1,
		double max = 131072,
		int zoom_type = ZOOM_PERCENTAGE,
		const char *first_item_text = 0,
		VFrame **menu_images = 0,
		VFrame **tumbler_images = 0);
	ZoomPanel(BC_WindowBase *subwindow,
		double value, 
		int x, 
		int y, 
		int w, 
		double *user_table,
		int user_size,
		int zoom_type = ZOOM_PERCENTAGE,
		const char *first_item_text = 0,
		VFrame **menu_images = 0,
		VFrame **tumbler_images = 0);
	virtual ~ZoomPanel();

	virtual int handle_event() { return 1; };
	int get_w();
	void calculate_menu();
	static double adjust_zoom(double zoom, double min, double max);
	void update_menu();
	double get_value();
	char* get_text();
	char* value_to_text(double value, int use_table = 1);
	double text_to_zoom(const char *text);
	void update(double value);
	void update(const char *value);
	void reposition_window(int x, int y);

	BC_WindowBase *subwindow;
	int x;
	int y;
	int w;
	double value;

	ZoomPopup *zoom_text;
	ZoomTumbler *zoom_tumbler;
	char string[BCTEXTLEN];
	double min, max;
	double *user_table;
	int user_size;
	int zoom_type;
	ArrayList<ZoomHash*> zoom_table;
	VFrame **menu_images;
	VFrame **tumbler_images;

private:
	void initialize(const char *first_item_text);
};

class ZoomPopup : public BC_PopupMenu
{
public:
	ZoomPopup(ZoomPanel *panel, int x, int y);

	int handle_event();

	ZoomPanel *panel;
};

class ZoomTumbler : public BC_Tumbler
{
public:
	ZoomTumbler(ZoomPanel *panel, int x, int y);

	void handle_up_event();
	void handle_down_event();

	ZoomPanel *panel;
};

#endif
