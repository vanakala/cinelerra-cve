#include "mainundo.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "plugin.h"
#include "plugindialog.h"
#include "pluginpopup.h"
#include "track.h"
#include "keyframe.h"
#include "edl.h"
#include "localsession.h"
#include "cwindow.h"

#include <libintl.h>
#define _(String) gettext(String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)


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
	add_item(attach = new PluginPopupAttach(mwindow, this));
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









PluginPopupAttach::PluginPopupAttach(MWindow *mwindow, PluginPopup *popup)
 : BC_MenuItem(_("Attach..."))
{
	this->mwindow = mwindow;
	this->popup = popup;
	dialog_thread = new PluginDialogThread(mwindow);
}

PluginPopupAttach::~PluginPopupAttach()
{
	delete dialog_thread;
}

int PluginPopupAttach::handle_event()
{
	dialog_thread->start_window(popup->plugin->track,
		popup->plugin, 
		PROGRAM_NAME ": Attach Effect");
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
	mwindow->undo->update_undo_before(_("detach effect"), LOAD_ALL);
	popup->plugin->track->detach_effect(popup->plugin);
	mwindow->save_backup();
	mwindow->undo->update_undo_after();
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


KeyframePopup::KeyframePopup(MWindow *mwindow, MWindowGUI *gui)
 : BC_PopupMenu(0, 
		0, 
		0, 
		"", 
		0)
{
	this->mwindow = mwindow;
	this->gui = gui;
}

KeyframePopup::~KeyframePopup()
{
}

void KeyframePopup::create_objects()
{
	add_item(key_delete = new KeyframePopupDelete(mwindow, this));
	add_item(key_show = new KeyframePopupShow(mwindow, this));
	add_item(key_copy = new KeyframePopupCopy(mwindow, this));
}

int KeyframePopup::update(Plugin *plugin, KeyFrame *keyframe)
{
	this->plugin = plugin;
	this->keyframe = keyframe;
	return 0;
}

KeyframePopupDelete::KeyframePopupDelete(MWindow *mwindow, KeyframePopup *popup)
 : BC_MenuItem(_("Delete keyframe"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

KeyframePopupDelete::~KeyframePopupDelete()
{
}

int KeyframePopupDelete::handle_event()
{
	mwindow->undo->update_undo_before(_("delete keyframe"), LOAD_ALL);
	delete popup->keyframe;
	mwindow->save_backup();
	mwindow->undo->update_undo_after();

	mwindow->gui->update(0,
	        1,      // 1 for incremental drawing.  2 for full refresh
	        0,
	        0,
	        0,
                0,   
                0);
	mwindow->update_plugin_guis();
	mwindow->restart_brender();
	mwindow->sync_parameters(CHANGE_EDL);

	return 1;
}

KeyframePopupShow::KeyframePopupShow(MWindow *mwindow, KeyframePopup *popup)
 : BC_MenuItem(_("Show keyframe settings"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

KeyframePopupShow::~KeyframePopupShow()
{
}

int KeyframePopupShow::handle_event()
{
/*	FIXME: this is currently not needed. but correct behaviour for maincursor is not snap to 
	position of the keyframe when right mouseclick is done
	snap should happen inside this rutine when 'show' is chosen
	snap should be disabled in trackcanvas.C

	double current_position = mwindow->edl->local_session->selectionstart;
	double new_position = popup->plugin->track->from_units(popup->keyframe->position);

	mwindow->edl->local_session->selectionstart = new_position;
	mwindow->edl->local_session->selectionend = new_position;
	mwindow->gui->cursor->hide();
	mwindow->gui->cursor->draw();
//	if (current_position != new_position)
	mwindow->cwindow->update(1, 0, 0, 0, 1);			
*/
	mwindow->update_plugin_guis();
	mwindow->show_plugin(popup->plugin);
	return 1;
}



KeyframePopupCopy::KeyframePopupCopy(MWindow *mwindow, KeyframePopup *popup)
 : BC_MenuItem(_("Copy keyframe"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

KeyframePopupCopy::~KeyframePopupCopy()
{
}

int KeyframePopupCopy::handle_event()
{
/*
	FIXME:
	we want to copy just keyframe under cursor, NOT all keyframes at this frame
	- very hard to do, so this is good approximation for now...
*/
	mwindow->copy_automation();
	return 1;
}



