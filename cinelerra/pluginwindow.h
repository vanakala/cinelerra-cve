// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2015 Einar Rünkaru <einarrunkaru at gmail dot com>

#ifndef PLUGINWINDOW_H
#define PLUGINWINDOW_H

#include "bcwindow.h"
#include "pluginclient.inc"

class PluginWindow : public BC_Window
{
public:
	PluginWindow(PluginClient *pluginclient, int x, int y, int w, int h);

	virtual void update() {};
	void close_event();
	BC_Title *print_title(int x, int y, const char *fmt, ...)
		__attribute__ ((format (printf, 4, 5)));

	int window_done;
private:
	PluginClient *pluginclient;
};
#endif
