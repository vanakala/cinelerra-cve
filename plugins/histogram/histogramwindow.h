
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

#ifndef HISTOGRAMWINDOW_H
#define HISTOGRAMWINDOW_H



#include "histogram.inc"
#include "histogramwindow.inc"
#include "pluginvclient.h"



class HistogramSlider : public BC_SubWindow
{
public:
	HistogramSlider(HistogramMain *plugin, 
		HistogramWindow *gui,
		int x, 
		int y, 
		int w,
		int h,
		int is_input);

	void update();
	int button_press_event();
	int button_release_event();
	int cursor_motion_event();
	int input_to_pixel(float input);

	int operation;
	enum
	{
		NONE,
		DRAG_INPUT,
		DRAG_MIN_OUTPUT,
		DRAG_MAX_OUTPUT,
	};
	int is_input;
	HistogramMain *plugin;
	HistogramWindow *gui;
};

class HistogramAuto : public BC_CheckBox
{
public:
	HistogramAuto(HistogramMain *plugin, 
		int x, 
		int y);
	int handle_event();
	HistogramMain *plugin;
};

class HistogramPlot : public BC_CheckBox
{
public:
	HistogramPlot(HistogramMain *plugin, 
		int x, 
		int y);
	int handle_event();
	HistogramMain *plugin;
};

class HistogramSplit : public BC_CheckBox
{
public:
	HistogramSplit(HistogramMain *plugin, 
		int x, 
		int y);
	int handle_event();
	HistogramMain *plugin;
};

class HistogramMode : public BC_Radial
{
public:
	HistogramMode(HistogramMain *plugin, 
		int x, 
		int y,
		int value,
		char *text);
	int handle_event();
	HistogramMain *plugin;
	int value;
};

class HistogramReset : public BC_GenericButton
{
public:
	HistogramReset(HistogramMain *plugin, 
		int x,
		int y);
	int handle_event();
	HistogramMain *plugin;
};

class HistogramOutputText : public BC_TumbleTextBox
{
public:
	HistogramOutputText(HistogramMain *plugin,
		HistogramWindow *gui,
		int x,
		int y,
		float *output);
	int handle_event();
	HistogramMain *plugin;
	float *output;
};

class HistogramInputText : public BC_TumbleTextBox
{
public:
	HistogramInputText(HistogramMain *plugin,
		HistogramWindow *gui,
		int x,
		int y,
		int do_x);

	int handle_event();
	void update();

	HistogramMain *plugin;
	HistogramWindow *gui;
	int do_x;
};

class HistogramCanvas : public BC_SubWindow
{
public:
	HistogramCanvas(HistogramMain *plugin,
		HistogramWindow *gui,
		int x,
		int y,
		int w,
		int h);
	int button_press_event();
	int cursor_motion_event();
	int button_release_event();
	HistogramMain *plugin;
	HistogramWindow *gui;
};

class HistogramWindow : public BC_Window
{
public:
	HistogramWindow(HistogramMain *plugin, int x, int y);
	~HistogramWindow();

	int create_objects();
	int close_event();
	void update(int do_input);
	void update_mode();
	void update_canvas();
	void draw_canvas_overlay();
	void update_input();
	void update_output();
	int keypress_event();
	void get_point_extents(HistogramPoint *current,
		int *x1, 
		int *y1, 
		int *x2, 
		int *y2,
		int *x,
		int *y);

	HistogramSlider *output;
	HistogramAuto *automatic;
	HistogramMode *mode_v, *mode_r, *mode_g, *mode_b /*,  *mode_a */;
	HistogramOutputText *output_min;
	HistogramOutputText *output_max;
	HistogramOutputText *threshold;
	HistogramInputText *input_x;
	HistogramInputText *input_y;
	HistogramCanvas *canvas;
	HistogramMain *plugin;
	int canvas_w;
	int canvas_h;
	int title1_x;
	int title2_x;
	int title3_x;
	int title4_x;
	BC_Pixmap *max_picon, *mid_picon, *min_picon;
	HistogramPlot *plot;
	HistogramSplit *split;
};



PLUGIN_THREAD_HEADER(HistogramMain, HistogramThread, HistogramWindow)




#endif
