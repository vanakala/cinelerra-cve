// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef IVTCWINDOW_H
#define IVTCWINDOW_H

#include "bctextbox.h"
#include "bctoggle.h"
#include "ivtc.h"
#include "pluginwindow.h"

#define TOTAL_PATTERNS 3

PLUGIN_THREAD_HEADER

class IVTCOffset;
class IVTCFieldOrder;
class IVTCAuto;
class IVTCAutoThreshold;
class IVTCPattern;

class IVTCWindow : public PluginWindow
{
public:
	IVTCWindow(IVTCMain *plugin, int x, int y);

	void update();

	IVTCOffset *frame_offset;
	IVTCFieldOrder *first_field;
	IVTCAutoThreshold *threshold;
	IVTCPattern *pattern[TOTAL_PATTERNS];
	PLUGIN_GUI_CLASS_MEMBERS
};

class IVTCOffset : public BC_TextBox
{
public:
	IVTCOffset(IVTCMain *client, int x, int y);

	int handle_event();
	IVTCMain *client;
};

class IVTCFieldOrder : public BC_CheckBox
{
public:
	IVTCFieldOrder(IVTCMain *client, int x, int y);

	int handle_event();
	IVTCMain *client;
};

class IVTCPattern : public BC_Radial
{
public:
	IVTCPattern(IVTCMain *client, 
		IVTCWindow *window, 
		int number, 
		const char *text,
		int x, 
		int y);

	int handle_event();

	IVTCWindow *window;
	IVTCMain *client;
	int number;
};

#endif
