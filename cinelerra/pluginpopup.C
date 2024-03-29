// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "cinelerra.h"
#include "bcsignals.h"
#include "bctitle.h"
#include "language.h"
#include "mainundo.h"
#include "mwindow.h"
#include "plugin.h"
#include "plugindialog.h"
#include "pluginpopup.h"
#include "track.h"


PluginPopup::PluginPopup()
 : BC_PopupMenu(0, 0, 0)
{
	add_item(plugin_title = new BC_MenuItem("-"));
	add_item(new BC_MenuItem("-"));
	add_item(change = new PluginPopupChange(this));
	add_item(detach = new PluginPopupDetach(this));
	add_item(new PluginPopupClearKeyFrames(this));
	show = new PluginPopupShow(this);
	add_item(on = new PluginPopupOn(this));
	moveup = new PluginPopupUp(this);
	movedown = new PluginPopupDown(this);
	pastekeyframe = new PluginPopupPasteKeyFrame(this);
	swapmain = new PluginPopupSwapMain(this);
	have_show = 0;
	have_keyframe = 0;
	have_moveup = 0;
	have_movedown = 0;
	have_swapmain = 0;
}

PluginPopup::~PluginPopup()
{
	if(!have_show)
		delete show;
	if(!have_keyframe)
		delete pastekeyframe;
	if(!have_moveup)
		delete moveup;
	if(!have_movedown)
		delete movedown;
	if(!have_swapmain)
		delete swapmain;
}

void PluginPopup::update(Plugin *plugin)
{
	char string[BCTEXTLEN];

	on->set_checked(plugin->on);
	show->set_checked(plugin->show);
	this->plugin = plugin;
	if(plugin->plugin_type == PLUGIN_STANDALONE)
	{
		plugin_title->set_text(plugin->calculate_title(string));
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
	if(plugin->plugin_type == PLUGIN_STANDALONE &&
		mwindow_global->can_paste_keyframe(plugin->track, plugin))
	{
		if(!have_keyframe)
		{
			add_item(pastekeyframe);
			have_keyframe = 1;
		}
	}
	else
	{
		remove_item(pastekeyframe);
		have_keyframe = 0;
	}

	if(plugin->track->plugins.total < 2 ||
		(plugin->is_multichannel() &&
		plugin->get_multichannel_count(plugin->get_pts(), plugin->end_pts()) > 1))
	{
		remove_item(moveup);
		remove_item(movedown);
		have_movedown = have_moveup = 0;
	}
	else
	{
		if(!have_moveup)
		{
			add_item(moveup);
			have_moveup = 1;
		}
		if(!have_movedown)
		{
			add_item(movedown);
			have_movedown = 1;
		}
	}
	if(plugin->plugin_type == PLUGIN_SHAREDPLUGIN)
	{
		plugin_title->set_text(plugin->calculate_title(string));
		if(!have_swapmain)
		{
			add_item(swapmain);
			have_swapmain = 1;
		}
	}
	else
	{
		if(have_swapmain)
		{
			remove_item(swapmain);
			have_swapmain = 0;
		}
	}
}


PluginPopupChange::PluginPopupChange(PluginPopup *popup)
 : BC_MenuItem(_("Change..."))
{
	this->popup = popup;
	dialog_thread = new PluginDialogThread();
}

PluginPopupChange::~PluginPopupChange()
{
	delete dialog_thread;
}

int PluginPopupChange::handle_event()
{
	dialog_thread->start_window(popup->plugin->track,
		popup->plugin);
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

	popup->plugin->track->remove_plugin(popup->plugin);
	mwindow_global->save_backup();
	mwindow_global->undo->update_undo(_("detach effect"), LOAD_ALL);
	mwindow_global->update_gui(WUPD_CANVINCR);
	mwindow_global->sync_parameters();
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
		popup->plugin->hide_plugin_gui();
	mwindow_global->update_gui(WUPD_CANVINCR);
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
	mwindow_global->update_gui(WUPD_CANVINCR);
	popup->plugin->update_toggles();
	mwindow_global->sync_parameters();
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

PluginPopupPasteKeyFrame::PluginPopupPasteKeyFrame(PluginPopup *popup)
 : BC_MenuItem(_("Paste keyframe"))
{
	this->popup = popup;
}

int PluginPopupPasteKeyFrame::handle_event()
{
	mwindow_global->paste_keyframe(popup->plugin->track, popup->plugin);
	return 1;
}

PluginPopupClearKeyFrames::PluginPopupClearKeyFrames(PluginPopup *popup)
 : BC_MenuItem(_("Clear keyframes"))
{
	this->popup = popup;
}

int PluginPopupClearKeyFrames::handle_event()
{
	mwindow_global->clear_keyframes(popup->plugin);
	return 1;
}

PluginPopupSwapMain::PluginPopupSwapMain(PluginPopup *popup)
 : BC_MenuItem(_("Swap with main"))
{
	this->popup = popup;
}

int PluginPopupSwapMain::handle_event()
{
	mwindow_global->swap_shared_main(popup->plugin);
	return 1;
}
