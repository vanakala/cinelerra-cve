// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

// Blue Dot theme by Koen Muylkens <koen.muylkens@esat.kuleuven.ac.be>

#ifndef BLUEDOTTHEME_H
#define BLUEDOTTHEME_H

#include "new.inc"
#include "plugintclient.h"
#include "preferencesthread.inc"
#include "statusbar.inc"
#include "theme.h"
#include "timebar.inc"

class BlueDotTheme : public Theme
{
public:
	BlueDotTheme();

	void initialize();
	void draw_mwindow_bg(MWindowGUI *gui);
	void draw_cwindow_bg(CWindowGUI *gui);
	void draw_vwindow_bg(VWindowGUI *gui);
	void draw_preferences_bg(PreferencesWindow *gui);

	void get_mwindow_sizes(MWindowGUI *gui, int w, int h);
	void get_cwindow_sizes(CWindowGUI *gui, int cwindow_controls);
	void get_vwindow_sizes(VWindowGUI *gui);
	void draw_new_bg(NewWindow *gui);
	void draw_setformat_bg(SetFormatWindow *gui);

private:
	void build_icons();
	void build_bg_data();
	void build_patches();
	void build_overlays();

// Record windows
	VFrame *rgui_batch;
	VFrame *rgui_controls;
	VFrame *rgui_list;
	VFrame *rmonitor_panel;
	VFrame *rmonitor_meters;
};


class BlueDotThemeMain : public PluginTClient
{
public:
	BlueDotThemeMain(PluginServer *server);

	const char* plugin_title();
	Theme* new_theme();

	BlueDotTheme *theme;
};

#endif
