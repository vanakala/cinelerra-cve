// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 1997-2011 Adam Williams <broadcast at earthling dot net>

#ifndef COLOR3WAYWINDOW_H
#define COLOR3WAYWINDOW_H

class Color3WayWindow;

#include "bcslider.h"
#include "color3way.h"
#include "pluginclient.h"
#include "pluginwindow.h"

PLUGIN_THREAD_HEADER

class Color3WayPoint : public BC_SubWindow
{
public:
	Color3WayPoint(Color3WayMain *plugin, Color3WayWindow *gui,
		double *x_output, double *y_output,
		int x, int y,
		int radius, int section);

	~Color3WayPoint();

	void update();
	void initialize();
	int cursor_enter_event();
	void cursor_leave_event();
	int cursor_motion_event();
	int button_release_event();
	int button_press_event();
	void draw_face(int flash, int flush);
	void reposition_window(int x, int y, int radius);
	void deactivate();
	void activate();
	int keypress_event();

	enum
	{
		COLOR_UP,
		COLOR_HI,
		COLOR_DN,
		COLOR_IMAGES
	};

	int active;
	int status;
	int fg_x;
	int fg_y;
	int starting_x;
	int starting_y;
	int offset_x;
	int offset_y;
	int drag_operation;
	Color3WayMain *plugin;
	Color3WayWindow *gui;
	int section;

	double *x_output;
	double *y_output;
	int radius;
	BC_Pixmap *fg_images[COLOR_IMAGES];
	BC_Pixmap *bg_image;
};


class Color3WaySlider : public BC_FSlider
{
public:
	Color3WaySlider(Color3WayMain *plugin, Color3WayWindow *gui,
		double *output, int x, int y, int w, int section);

	int handle_event();
	char* get_caption();

private:
	Color3WayMain *plugin;
	Color3WayWindow *gui;
	double *output;
	double old_value;
	int section;
	char string[BCTEXTLEN];
};


class Color3WayResetSection : public BC_GenericButton
{
public:
	Color3WayResetSection(Color3WayMain *plugin, Color3WayWindow *gui, 
		int x, int y, int section);

	int handle_event();

private:
	Color3WayMain *plugin;
	Color3WayWindow *gui;
	int section;
};


class Color3WayCopySection : public BC_CheckBox
{
public:
	Color3WayCopySection(Color3WayMain *plugin, Color3WayWindow *gui,
		int x, int y, int section);

	int handle_event();

private:
	Color3WayMain *plugin;
	Color3WayWindow *gui;
	int section;
};


class Color3WaySection
{
public:
	Color3WaySection(Color3WayMain *plugin, Color3WayWindow *gui,
		int x, int y, int w, int h, int section);

	void reposition_window(int x, int y, int w, int h);
	void update();

	int x, y, w, h;

private:
	BC_Title *title;
	Color3WayMain *plugin;
	Color3WayWindow *gui;
	int section;
	Color3WayPoint *point;
	BC_Title *value_title;
	Color3WaySlider *value;
	BC_Title *sat_title;
	Color3WaySlider *saturation;
	Color3WayResetSection *reset;
	Color3WayCopySection *copy;
};


class Color3WayWindow : public PluginWindow
{
public:
	Color3WayWindow(Color3WayMain *plugin, int x, int y);

	void update();

	Color3WayPoint *active_point;
	Color3WaySection *sections[SECTIONS];
	PLUGIN_GUI_CLASS_MEMBERS
};

#endif
