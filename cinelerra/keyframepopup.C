#include "cwindow.h"
#include "edl.h"
#include "keyframe.h"
#include "keyframepopup.h"
#include "language.h"
#include "mainundo.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "localsession.h"
 
 

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



