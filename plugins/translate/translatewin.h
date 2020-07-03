// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef TRANSLATEWIN_H
#define TRANSLATEWIN_H

#include "bctextbox.h"
#include "translate.h"
#include "pluginclient.h"
#include "pluginwindow.h"

PLUGIN_THREAD_HEADER

class TranslateCoord;

class TranslateWin : public PluginWindow
{
public:
	TranslateWin(TranslateMain *plugin, int x, int y);

	void update();

	TranslateCoord *in_x, *in_y, *in_w, *in_h, *out_x, *out_y, *out_w, *out_h;
	PLUGIN_GUI_CLASS_MEMBERS
};

class TranslateCoord : public BC_TumbleTextBox
{
public:
	TranslateCoord(TranslateWin *win,
		TranslateMain *client,
		int x, int y,
		double *value);

	int handle_event();

	TranslateMain *client;
	double *value;
};

#endif
