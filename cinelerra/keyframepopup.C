
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

#include "cwindow.h"
#include "edl.h"
#include "keyframe.h"
#include "keyframepopup.h"
#include "language.h"
#include "mainundo.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "localsession.h"
#include "cwindowgui.h" 
#include "cpanel.h"
#include "patchbay.h"
#include "patchgui.h" 
#include "apatchgui.h"
#include "vpatchgui.h"
#include "track.h"
#include "maincursor.h"
#include "bcwindowbase.h"
#include "filexml.h"
#include "edlsession.h"
#include "autoconf.h"

KeyframePopup::KeyframePopup(MWindow *mwindow, MWindowGUI *gui)
 : BC_PopupMenu(0, 
		0, 
		0, 
		"", 
		0)
{
	this->mwindow = mwindow;
	this->gui = gui;
	key_delete = 0;
	key_copy = 0;
}

KeyframePopup::~KeyframePopup()
{
}

void KeyframePopup::create_objects()
{
	add_item(key_delete = new KeyframePopupDelete(mwindow, this));
	add_item(key_copy = new KeyframePopupCopy(mwindow, this));
}

int KeyframePopup::update(Plugin *plugin, KeyFrame *keyframe)
{
	this->keyframe_plugin = plugin;
	this->keyframe_auto = keyframe;
	this->keyframe_autos = 0;
	this->keyframe_automation = 0;
	return 0;
}

int KeyframePopup::update(Automation *automation, Autos *autos, Auto *auto_keyframe)
{
	this->keyframe_plugin = 0;
	this->keyframe_automation = automation;
	this->keyframe_autos = autos;
	this->keyframe_auto = auto_keyframe;

	/* FIXPOS snap to cursor */
	double current_position = mwindow->edl->local_session->get_selectionstart(1);
	double new_position = keyframe_auto->pos_time;
	mwindow->edl->local_session->set_selectionstart(new_position);
	mwindow->edl->local_session->set_selectionend(new_position);
	if (current_position != new_position)
	{
		mwindow->edl->local_session->set_selectionstart(new_position);
		mwindow->edl->local_session->set_selectionend(new_position);
		mwindow->gui->lock_window("KeyframePopup::update");
		mwindow->gui->update(1, 1, 1, 1, 1, 1, 0);
		mwindow->gui->unlock_window();
	}
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
	delete popup->keyframe_auto;
	mwindow->save_backup();
	mwindow->undo->update_undo(_("delete keyframe"), LOAD_ALL);

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
//	FIXME:
//	we want to copy just keyframe under cursor, NOT all keyframes at this frame
//	- very hard to do, so this is good approximation for now...

	mwindow->copy_automation();
	return 1;
}



