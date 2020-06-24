// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef COLORBALANCEWINDOW_H
#define COLORBALANCEWINDOW_H

class ColorBalanceSlider;
class ColorBalancePreserve;
class ColorBalanceLock;
class ColorBalanceWhite;
class ColorBalanceReset;

#include "bcbutton.h"
#include "bcslider.h"
#include "bctoggle.h"
#include "colorbalance.h"
#include "pluginclient.h"
#include "pluginwindow.h"

PLUGIN_THREAD_HEADER

class ColorBalanceWindow : public PluginWindow
{
public:
	ColorBalanceWindow(ColorBalanceMain *plugin, int x, int y);

	void update();

	ColorBalanceSlider *cyan;
	ColorBalanceSlider *magenta;
	ColorBalanceSlider *yellow;
	ColorBalanceLock *lock_params;
	ColorBalancePreserve *preserve;
	ColorBalanceWhite *use_eyedrop;
	ColorBalanceReset *reset;
	PLUGIN_GUI_CLASS_MEMBERS
};

class ColorBalanceSlider : public BC_ISlider
{
public:
	ColorBalanceSlider(ColorBalanceMain *client, double *output, int x, int y);

	int handle_event();
	char* get_caption();

	ColorBalanceMain *client;
	double *output;
	char string[64];
};

class ColorBalancePreserve : public BC_CheckBox
{
public:
	ColorBalancePreserve(ColorBalanceMain *client, int x, int y);

	int handle_event();

	ColorBalanceMain *client;
};

class ColorBalanceLock : public BC_CheckBox
{
public:
	ColorBalanceLock(ColorBalanceMain *client, int x, int y);

	int handle_event();

	ColorBalanceMain *client;
};

class ColorBalanceWhite : public BC_GenericButton
{
public:
	ColorBalanceWhite(ColorBalanceMain *plugin, ColorBalanceWindow *gui,
		int x, int y);

	int handle_event();

	ColorBalanceMain *plugin;
	ColorBalanceWindow *gui;
};

class ColorBalanceReset : public BC_GenericButton
{
public:
	ColorBalanceReset(ColorBalanceMain *plugin, ColorBalanceWindow *gui,
		int x, int y);

	int handle_event();

	ColorBalanceMain *plugin;
	ColorBalanceWindow *gui;
};

#endif
