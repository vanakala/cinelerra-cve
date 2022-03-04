// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "autoconf.h"
#include "cinelerra.h"
#include "edl.h"
#include "edlsession.h"
#include "gwindow.h"
#include "gwindowgui.h"
#include "language.h"
#include "mainmenu.h"
#include "mwindow.h"
#include "viewmenu.h"


ShowAssets::ShowAssets(const char *hotkey)
 : BC_MenuItem(_("Show assets"), hotkey, hotkey[0])
{
	set_checked(edlsession->show_assets);
}

int ShowAssets::handle_event()
{
	set_checked(!get_checked());
	edlsession->show_assets = get_checked();
	mwindow_global->update_gui(WUPD_SCROLLBARS | WUPD_CANVINCR | WUPD_PATCHBAY);
	mwindow_global->gwindow->gui->update_toggles();
	return 1;
}


ShowTitles::ShowTitles(const char *hotkey)
 : BC_MenuItem(_("Show titles"), hotkey, hotkey[0])
{
	set_checked(edlsession->show_titles);
}

int ShowTitles::handle_event()
{
	set_checked(!get_checked());
	edlsession->show_titles = get_checked();
	mwindow_global->update_gui(WUPD_SCROLLBARS | WUPD_CANVINCR | WUPD_PATCHBAY);
	mwindow_global->gwindow->gui->update_toggles();
	return 1;
}

ShowTransitions::ShowTransitions(const char *hotkey)
 : BC_MenuItem(_("Show transitions"), hotkey, hotkey[0])
{ 
	set_checked(edlsession->auto_conf->transitions_visible);
}

int ShowTransitions::handle_event()
{
	set_checked(!get_checked());
	edlsession->auto_conf->transitions_visible = get_checked();
	mwindow_global->draw_canvas_overlays();
	mwindow_global->gwindow->gui->update_toggles();
	return 1;
}


ShowAutomation::ShowAutomation(const char *text,
	const char *hotkey,
	int subscript)
 : BC_MenuItem(text, hotkey, hotkey[0])
{
	this->subscript = subscript;
	set_checked(edlsession->auto_conf->auto_visible[subscript]);
}

int ShowAutomation::handle_event()
{
	set_checked(!get_checked());
	edlsession->auto_conf->auto_visible[subscript] = get_checked();
	mwindow_global->draw_canvas_overlays();
	mwindow_global->gwindow->gui->update_toggles();
	return 1;
}

void ShowAutomation::update_toggle()
{
	set_checked(edlsession->auto_conf->auto_visible[subscript]);
}


PluginAutomation::PluginAutomation(const char *hotkey)
 : BC_MenuItem(_("Plugin Autos"), hotkey, hotkey[0])
{
}

int PluginAutomation::handle_event()
{
	set_checked(!get_checked());
	edlsession->keyframes_visible = get_checked();
	mwindow_global->draw_canvas_overlays();
	mwindow_global->gwindow->gui->update_toggles();
	return 1;
}
