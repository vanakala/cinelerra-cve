// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef LINEARIZEWINDOW_H
#define LINEARIZEWINDOW_H

class MaxSlider;
class MaxText;
class GammaSlider;
class GammaText;
class GammaAuto;
class GammaPlot;
class GammaColorPicker;

#include "bcslider.h"
#include "bctextbox.h"
#include "bctoggle.h"
#include "filexml.h"
#include "mutex.h"
#include "gamma.h"
#include "pluginclient.h"
#include "pluginwindow.h"

PLUGIN_THREAD_HEADER

class GammaWindow : public PluginWindow
{
public:
	GammaWindow(GammaMain *plugin, int x, int y);

	void update();
	void update_histogram();

	BC_SubWindow *histogram;
	MaxSlider *max_slider;
	MaxText *max_text;
	GammaSlider *gamma_slider;
	GammaText *gamma_text;
	GammaAuto *automatic;
	GammaPlot *plot;
	PLUGIN_GUI_CLASS_MEMBERS
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
