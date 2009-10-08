
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

#include "autoconf.h"
#include "edl.h"
#include "edlsession.h"
#include "gwindow.h"
#include "gwindowgui.h"
#include "language.h"
#include "mainmenu.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "viewmenu.h"
#include "trackcanvas.h"





ShowAssets::ShowAssets(MWindow *mwindow, char *hotkey)
 : BC_MenuItem(_("Show assets"), hotkey, hotkey[0])
{
	this->mwindow = mwindow; 
	set_checked(mwindow->edl->session->show_assets); 
}

int ShowAssets::handle_event()
{
	set_checked(get_checked() ^ 1);
	mwindow->edl->session->show_assets = get_checked();
	mwindow->gui->update(1,
		1,
		0,
		0,
		1, 
		0,
		0);
	mwindow->gui->unlock_window();
	mwindow->gwindow->gui->update_toggles(1);
	mwindow->gui->lock_window("ShowAssets::handle_event");
	return 1;
}




ShowTitles::ShowTitles(MWindow *mwindow, char *hotkey)
 : BC_MenuItem(_("Show titles"), hotkey, hotkey[0])
{
	this->mwindow = mwindow; 
	set_checked(mwindow->edl->session->show_titles); 
}

int ShowTitles::handle_event()
{
	set_checked(get_checked() ^ 1);
	mwindow->edl->session->show_titles = get_checked();
	mwindow->gui->update(1,
		1,
		0,
		0,
		1, 
		0,
		0);
	mwindow->gui->unlock_window();
	mwindow->gwindow->gui->update_toggles(1);
	mwindow->gui->lock_window("ShowTitles::handle_event");
	return 1;
}



ShowTransitions::ShowTransitions(MWindow *mwindow, char *hotkey)
 : BC_MenuItem(_("Show transitions"), hotkey, hotkey[0])
{ 
	this->mwindow = mwindow; 
	set_checked(mwindow->edl->session->auto_conf->transitions); 
}
int ShowTransitions::handle_event()
{
	set_checked(get_checked() ^ 1);
	mwindow->edl->session->auto_conf->transitions = get_checked();
	mwindow->gui->canvas->draw_overlays();
	mwindow->gui->canvas->flash();
//	mwindow->gui->mainmenu->draw_items();
	mwindow->gui->unlock_window();
	mwindow->gwindow->gui->update_toggles(1);
	mwindow->gui->lock_window("ShowTransitions::handle_event");
	return 1;
}





ShowAutomation::ShowAutomation(MWindow *mwindow, 
	char *text,
	char *hotkey,
	int subscript)
 : BC_MenuItem(text, hotkey, hotkey[0])
{
	this->mwindow = mwindow;
	this->subscript = subscript;
	set_checked(mwindow->edl->session->auto_conf->autos[subscript]); 
}

int ShowAutomation::handle_event()
{
	set_checked(get_checked() ^ 1);
	mwindow->edl->session->auto_conf->autos[subscript] = get_checked();
	mwindow->gui->canvas->draw_overlays();
	mwindow->gui->canvas->flash();
//	mwindow->gui->mainmenu->draw_items();
	mwindow->gui->unlock_window();
	mwindow->gwindow->gui->update_toggles(1);
	mwindow->gui->lock_window("ShowAutomation::handle_event");
	return 1;
}

void ShowAutomation::update_toggle()
{
	set_checked(mwindow->edl->session->auto_conf->autos[subscript]);
}



PluginAutomation::PluginAutomation(MWindow *mwindow, char *hotkey)
 : BC_MenuItem(_("Plugin Autos"), hotkey, hotkey[0]) 
{ 
	this->mwindow = mwindow; 
}

int PluginAutomation::handle_event()
{
	set_checked(!get_checked());
	mwindow->edl->session->auto_conf->plugins = get_checked();
	mwindow->gui->canvas->draw_overlays();
	mwindow->gui->canvas->flash();
//	mwindow->gui->mainmenu->draw_items();
	mwindow->gui->unlock_window();
	mwindow->gwindow->gui->update_toggles(1);
	mwindow->gui->lock_window("PluginAutomation::handle_event");
	return 1;
}


