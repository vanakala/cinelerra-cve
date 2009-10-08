
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

#ifndef THRESHOLDWINDOW_H
#define THRESHOLDWINDOW_H

#include "colorpicker.h"
#include "guicast.h"
#include "pluginvclient.h"
#include "threshold.inc"
#include "thresholdwindow.inc"

class ThresholdMin : public BC_TumbleTextBox
{
public:
	ThresholdMin(ThresholdMain *plugin,
		ThresholdWindow *gui,
		int x,
		int y,
		int w);
	int handle_event();
	ThresholdMain *plugin;
	ThresholdWindow *gui;
};

class ThresholdMax : public BC_TumbleTextBox
{
public:
	ThresholdMax(ThresholdMain *plugin,
		ThresholdWindow *gui,
		int x,
		int y,
		int w);
	int handle_event();
	ThresholdMain *plugin;
	ThresholdWindow *gui;
};

class ThresholdPlot : public BC_CheckBox
{
public:
	ThresholdPlot(ThresholdMain *plugin,
		int x,
		int y);
	int handle_event();
	ThresholdMain *plugin;
};

class ThresholdLowColorButton : public BC_GenericButton
{
public:
	ThresholdLowColorButton(ThresholdMain *plugin, ThresholdWindow *window, int x, int y);
	int handle_event();
	ThresholdMain *plugin;
	ThresholdWindow *window;
};

class ThresholdMidColorButton : public BC_GenericButton
{
public:
	ThresholdMidColorButton(ThresholdMain *plugin, ThresholdWindow *window, int x, int y);
	int handle_event();
	ThresholdMain *plugin;
	ThresholdWindow *window;
};

class ThresholdHighColorButton : public BC_GenericButton
{
public:
	ThresholdHighColorButton(ThresholdMain *plugin, ThresholdWindow *window, int x, int y);
	int handle_event();
	ThresholdMain *plugin;
	ThresholdWindow *window;
};

class ThresholdLowColorThread : public ColorThread
{
public:
	ThresholdLowColorThread(ThresholdMain *plugin, ThresholdWindow *window);
	virtual int handle_new_color(int output, int alpha);
	ThresholdMain *plugin;
	ThresholdWindow *window;
};

class ThresholdMidColorThread : public ColorThread
{
public:
	ThresholdMidColorThread(ThresholdMain *plugin, ThresholdWindow *window);
	virtual int handle_new_color(int output, int alpha);
	ThresholdMain *plugin;
	ThresholdWindow *window;
};

class ThresholdHighColorThread : public ColorThread
{
public:
	ThresholdHighColorThread(ThresholdMain *plugin, ThresholdWindow *window);
	virtual int handle_new_color(int output, int alpha);
	ThresholdMain *plugin;
	ThresholdWindow *window;
};

class ThresholdCanvas : public BC_SubWindow
{
public:
	ThresholdCanvas(ThresholdMain *plugin,
		ThresholdWindow *gui,
		int x,
		int y,
		int w,
		int h);
	int button_press_event();
	int button_release_event();
	int cursor_motion_event();
	void draw();

	ThresholdMain *plugin;
	ThresholdWindow *gui;
	int state;
	enum
	{
		NO_OPERATION,
		DRAG_SELECTION
	};
	int x1;
	int x2;
	int center_x;
};

class ThresholdWindow : public BC_Window
{
public:
	ThresholdWindow(ThresholdMain *plugin, int x, int y);
	~ThresholdWindow();
	
	int create_objects();
	int close_event();
	void update_low_color();
	void update_mid_color();
	void update_high_color();

	ThresholdMain *plugin;
	ThresholdMin *min;
	ThresholdMax *max;
	ThresholdCanvas *canvas;
	ThresholdPlot *plot;
	ThresholdLowColorButton  *low_color;
	ThresholdMidColorButton  *mid_color;
	ThresholdHighColorButton *high_color;
	ThresholdLowColorThread  *low_color_thread;
	ThresholdMidColorThread  *mid_color_thread;
	ThresholdHighColorThread *high_color_thread;

	int low_color_x,  low_color_y;
	int mid_color_x,  mid_color_y;
	int high_color_x, high_color_y;
};

PLUGIN_THREAD_HEADER(ThresholdMain, ThresholdThread, ThresholdWindow)


#endif






