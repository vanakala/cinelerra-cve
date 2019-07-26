
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


PluginPopup::PluginPopup()
 : BC_PopupMenu(0, 
		0, 
		0, 
		"", 
		0)
{
	add_item(change = new PluginPopupChange(this));
	add_item(detach = new PluginPopupDetach(this));
	show = new PluginPopupShow(this);
	add_item(on = new PluginPopupOn(this));
	add_item(new PluginPopupUp(this));
	add_item(new PluginPopupDown(this));
	have_show = 0;
}

PluginPopup::~PluginPopup()
{
	if(!have_show)
		delete show;
}

void PluginPopup::update(Plugin *plugin)
{
	on->set_checked(plugin->on);
	show->set_checked(plugin->show);
	if(plugin->plugin_type == PLUGIN_STANDALONE)
	{
		if(!have_show)
		{
			add_item(show);
			have_show = 1;
		}
	}
	else
	{
		remove_item(show);
		have_show = 0;
	}
	this->plugin = plugin;
}


PluginPopupChange::PluginPopupChange(PluginPopup *popup)
 : BC_MenuItem(_("Change..."))
{
	this->popup = popup;
	dialog_thread = new PluginDialogThread(mwindow_global);
}

PluginPopupChange::~PluginPopupChange()
{
	delete dialog_thread;
}

int PluginPopupChange::handle_event()
{
	dialog_thread->start_window(popup->plugin->track,
		popup->plugin,
		MWindow::create_title(N_("Change Effect")));
	return 1;
}


PluginPopupDetach::PluginPopupDetach(PluginPopup *popup)
 : BC_MenuItem(_("Detach"))
{
	this->popup = popup;
}

int PluginPopupDetach::handle_event()
{
	if(mwindow_global->stop_composer())
		return 0;
	mwindow_global->hide_plugin(popup->plugin, 1);
	popup->plugin->track->remove_plugin(popup->plugin);
	mwindow_global->save_backup();
	mwindow_global->undo->update_undo(_("detach effect"), LOAD_ALL);
	mwindow_global->gui->update(WUPD_CANVINCR);
	mwindow_global->restart_brender();
	mwindow_global->sync_parameters(CHANGE_EDL);
	return 1;
}


PluginPopupShow::PluginPopupShow(PluginPopup *popup)
 : BC_MenuItem(_("Parameters"))
{
	this->popup = popup;
}

int PluginPopupShow::handle_event()
{
	if(!get_checked())
		mwindow_global->show_plugin(popup->plugin);
	else
		mwindow_global->hide_plugin(popup->plugin, 1);
	mwindow_global->gui->update(WUPD_CANVINCR);
	return 1;
}


PluginPopupOn::PluginPopupOn(PluginPopup *popup)
 : BC_MenuItem(_("On"))
{
	this->popup = popup;
}

int PluginPopupOn::handle_event()
{
	if(!popup->plugin->plugin_server)
	{
		popup->plugin->on = 0;
		return 0;
	}
	if(mwindow_global->stop_composer())
		return 0;
	popup->plugin->on = !get_checked();
	mwindow_global->gui->update(WUPD_CANVINCR);
	mwindow_global->restart_brender();
	mwindow_global->sync_parameters(CHANGE_EDL);
	return 1;
}


PluginPopupUp::PluginPopupUp(PluginPopup *popup)
 : BC_MenuItem(_("Move up"))
{
	this->popup = popup;
}

int PluginPopupUp::handle_event()
{
	mwindow_global->move_plugin_up(popup->plugin);
	return 1;
}


PluginPopupDown::PluginPopupDown(PluginPopup *popup)
 : BC_MenuItem(_("Move down"))
{
	this->popup = popup;
}

int PluginPopupDown::handle_event()
{
	mwindow_global->move_plugin_down(popup->plugin);
	return 1;
}
