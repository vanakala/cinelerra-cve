
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

#include "cinelerra.h"
#include "bcsignals.h"
#include "language.h"
#include "mainundo.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "plugin.h"
#include "plugindialog.h"
#include "pluginpopup.h"
#include "track.h"


PluginPopup::PluginPopup(MWindow *mwindow, MWindowGUI *gui)
 : BC_PopupMenu(0, 
		0, 
		0, 
		"", 
		0)
{
	this->mwindow = mwindow;
	this->gui = gui;
	add_item(change = new PluginPopupChange(mwindow, this));
	add_item(detach = new PluginPopupDetach(mwindow, this));
	add_item(show = new PluginPopupShow(mwindow, this));
	add_item(on = new PluginPopupOn(mwindow, this));
	add_item(new PluginPopupUp(mwindow, this));
	add_item(new PluginPopupDown(mwindow, this));
}

void PluginPopup::update(Plugin *plugin)
{
	on->set_checked(plugin->on);
	show->set_checked(plugin->show);
	this->plugin = plugin;
}


PluginPopupChange::PluginPopupChange(MWindow *mwindow, PluginPopup *popup)
 : BC_MenuItem(_("Change..."))
{
	this->mwindow = mwindow;
	this->popup = popup;
	dialog_thread = new PluginDialogThread(mwindow);
}

PluginPopupChange::~PluginPopupChange()
{
	delete dialog_thread;
}

int PluginPopupChange::handle_event()
{
	dialog_thread->start_window(popup->plugin->track,
		popup->plugin,
		"Change Effect - " PROGRAM_NAME);
	return 1;
}


PluginPopupDetach::PluginPopupDetach(MWindow *mwindow, PluginPopup *popup)
 : BC_MenuItem(_("Detach"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

int PluginPopupDetach::handle_event()
{
	mwindow->hide_plugin(popup->plugin, 1);
	popup->plugin->track->detach_effect(popup->plugin);
	mwindow->save_backup();
	mwindow->undo->update_undo(_("detach effect"), LOAD_ALL);
	mwindow->gui->update(WUPD_CANVINCR);
	mwindow->restart_brender();
	mwindow->sync_parameters(CHANGE_EDL);
	return 1;
}


PluginPopupShow::PluginPopupShow(MWindow *mwindow, PluginPopup *popup)
 : BC_MenuItem(_("Window"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

int PluginPopupShow::handle_event()
{
	if(!get_checked())
		mwindow->show_plugin(popup->plugin);
	else
		mwindow->hide_plugin(popup->plugin, 1);
	mwindow->gui->update(WUPD_CANVINCR);
	return 1;
}


PluginPopupOn::PluginPopupOn(MWindow *mwindow, PluginPopup *popup)
 : BC_MenuItem(_("On"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

int PluginPopupOn::handle_event()
{
	popup->plugin->on = !get_checked();
	mwindow->gui->update(WUPD_CANVINCR);
	mwindow->restart_brender();
	mwindow->sync_parameters(CHANGE_EDL);
	return 1;
}


PluginPopupUp::PluginPopupUp(MWindow *mwindow, PluginPopup *popup)
 : BC_MenuItem(_("Move up"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

int PluginPopupUp::handle_event()
{
	mwindow->move_plugins_up(popup->plugin->plugin_set);
	return 1;
}


PluginPopupDown::PluginPopupDown(MWindow *mwindow, PluginPopup *popup)
 : BC_MenuItem(_("Move down"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

int PluginPopupDown::handle_event()
{
	mwindow->move_plugins_down(popup->plugin->plugin_set);
	return 1;
}
