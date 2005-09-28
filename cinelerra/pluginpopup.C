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
}

PluginPopup::~PluginPopup()
{
}

void PluginPopup::create_objects()
{
	add_item(change = new PluginPopupChange(mwindow, this));
	add_item(detach = new PluginPopupDetach(mwindow, this));
//	add_item(in = new PluginPopupIn(mwindow, this));
//	add_item(out = new PluginPopupOut(mwindow, this));
	add_item(show = new PluginPopupShow(mwindow, this));
	add_item(on = new PluginPopupOn(mwindow, this));
	add_item(new PluginPopupUp(mwindow, this));
	add_item(new PluginPopupDown(mwindow, this));
}

int PluginPopup::update(Plugin *plugin)
{
//printf("PluginPopup::update %p\n", plugin);
	on->set_checked(plugin->on);
//	in->set_checked(plugin->in);
//	out->set_checked(plugin->out);
	show->set_checked(plugin->show);
	this->plugin = plugin;
	return 0;
}









PluginPopupChange::PluginPopupChange(MWindow *mwindow, PluginPopup
*popup)
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
		PROGRAM_NAME ": Change Effect");
}








PluginPopupDetach::PluginPopupDetach(MWindow *mwindow, PluginPopup *popup)
 : BC_MenuItem(_("Detach"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

PluginPopupDetach::~PluginPopupDetach()
{
}

int PluginPopupDetach::handle_event()
{
	mwindow->hide_plugin(popup->plugin, 1);
	popup->plugin->track->detach_effect(popup->plugin);
	mwindow->save_backup();
	mwindow->undo->update_undo(_("detach effect"), LOAD_ALL);
	mwindow->gui->update(0,
		1,
		0,
		0,
		0, 
		0,
		0);
	mwindow->restart_brender();
	mwindow->sync_parameters(CHANGE_EDL);
	return 1;
}







PluginPopupIn::PluginPopupIn(MWindow *mwindow, PluginPopup *popup)
 : BC_MenuItem(_("Send"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

PluginPopupIn::~PluginPopupIn()
{
}

int PluginPopupIn::handle_event()
{
	popup->plugin->in = !get_checked();
	mwindow->sync_parameters(CHANGE_EDL);
	return 1;
}





PluginPopupOut::PluginPopupOut(MWindow *mwindow, PluginPopup *popup)
 : BC_MenuItem(_("Receive"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

PluginPopupOut::~PluginPopupOut()
{
}

int PluginPopupOut::handle_event()
{
	popup->plugin->out = !get_checked();
	mwindow->sync_parameters(CHANGE_EDL);
	return 1;
}





PluginPopupShow::PluginPopupShow(MWindow *mwindow, PluginPopup *popup)
 : BC_MenuItem(_("Show"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

PluginPopupShow::~PluginPopupShow()
{
}

int PluginPopupShow::handle_event()
{
	mwindow->show_plugin(popup->plugin);
	return 1;
}




PluginPopupOn::PluginPopupOn(MWindow *mwindow, PluginPopup *popup)
 : BC_MenuItem(_("On"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

PluginPopupOn::~PluginPopupOn()
{
}

int PluginPopupOn::handle_event()
{
	popup->plugin->on = !get_checked();
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

