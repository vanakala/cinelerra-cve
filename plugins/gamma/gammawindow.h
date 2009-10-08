
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

#ifndef LINEARIZEWINDOW_H
#define LINEARIZEWINDOW_H


class GammaThread;
class GammaWindow;
class MaxSlider;
class MaxText;
class GammaSlider;
class GammaText;
class GammaAuto;
class GammaPlot;
class GammaColorPicker;

#include "filexml.h"
#include "guicast.h"
#include "mutex.h"
#include "gamma.h"
#include "pluginclient.h"


PLUGIN_THREAD_HEADER(GammaMain, GammaThread, GammaWindow)

class GammaWindow : public BC_Window
{
public:
	GammaWindow(GammaMain *client, int x, int y);

	int create_objects();
	int close_event();
	void update();
	void update_histogram();


	BC_SubWindow *histogram;
	GammaMain *client;
	MaxSlider *max_slider;
	MaxText *max_text;
	GammaSlider *gamma_slider;
	GammaText *gamma_text;
	GammaAuto *automatic;
	GammaPlot *plot;
};

class MaxSlider : public BC_FSlider
{
public:
	MaxSlider(GammaMain *client, 
		GammaWindow *gui, 
		int x, 
		int y,
		int w);
	int handle_event();
	GammaMain *client;
	GammaWindow *gui;
};

class MaxText : public BC_TextBox
{
public:
	MaxText(GammaMain *client,
		GammaWindow *gui,
		int x,
		int y,
		int w);
	int handle_event();
	GammaMain *client;
	GammaWindow *gui;
};

class GammaSlider : public BC_FSlider
{
public:
	GammaSlider(GammaMain *client, 
		GammaWindow *gui, 
		int x, 
		int y,
		int w);
	int handle_event();
	GammaMain *client;
	GammaWindow *gui;
};

class GammaText : public BC_TextBox
{
public:
	GammaText(GammaMain *client,
		GammaWindow *gui,
		int x,
		int y,
		int w);
	int handle_event();
	GammaMain *client;
	GammaWindow *gui;
};

class GammaAuto : public BC_CheckBox
{
public:
	GammaAuto(GammaMain *plugin, int x, int y);
	int handle_event();
	GammaMain *plugin;
};

class GammaPlot : public BC_CheckBox
{
public:
	GammaPlot(GammaMain *plugin, int x, int y);
	int handle_event();
	GammaMain *plugin;
};

class GammaColorPicker : public BC_GenericButton
{
public:
	GammaColorPicker(GammaMain *plugin, 
		GammaWindow *gui, 
		int x, 
		int y);
	int handle_event();
	GammaMain *plugin;
	GammaWindow *gui;
};

#endif
