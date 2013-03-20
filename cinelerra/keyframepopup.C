
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

#include "autos.h"
#include "bcsignals.h"
#include "edl.h"
#include "keyframe.h"
#include "keyframepopup.h"
#include "language.h"
#include "mainundo.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "localsession.h"
#include "track.h"
#include "bcwindowbase.h"

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
	delete_active = 0;
	key_copy = 0;
}

void KeyframePopup::create_objects()
{
	add_item(key_delete = new KeyframePopupDelete(mwindow, this));
	add_item(key_copy = new KeyframePopupCopy(mwindow, this));
	delete_active = 1;
}

void KeyframePopup::update(Plugin *plugin, KeyFrame *keyframe)
{
	this->keyframe_plugin = plugin;
	this->keyframe_auto = keyframe;
	this->keyframe_autos = 0;
	this->keyframe_automation = 0;
}

void KeyframePopup::update(Automation *automation, Autos *autos, Auto *auto_keyframe)
{
	this->keyframe_plugin = 0;
	this->keyframe_automation = automation;
	this->keyframe_autos = autos;
	this->keyframe_auto = auto_keyframe;

	/* FIXPOS snap to cursor */
	ptstime current_position = mwindow->edl->local_session->get_selectionstart(1);
	ptstime new_position = keyframe_auto->pos_time;
	if(autos->first == auto_keyframe)
	{
		if(delete_active)
		{
			remove_item(key_delete);
			delete_active = 0;
		}
	}
	else
	{
		if(!delete_active)
		{
			add_item(key_delete);
			delete_active = 1;
		}
	}

	mwindow->edl->local_session->set_selection(new_position);

	if(!PTSEQU(current_position, new_position))
	{
		mwindow->gui->lock_window("KeyframePopup::update");
		mwindow->gui->update(1, 1, 1, 1, 1, 1, 0);
		mwindow->gui->unlock_window();
	}
}

KeyframePopupDelete::KeyframePopupDelete(MWindow *mwindow, KeyframePopup *popup)
 : BC_MenuItem(_("Delete keyframe"))
{
	this->mwindow = mwindow;
	this->popup = popup;
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

int KeyframePopupCopy::handle_event()
{
//	FIXME:
//	we want to copy just keyframe under cursor, NOT all keyframes at this frame
//	- very hard to do, so this is good approximation for now...

	mwindow->copy_automation();
	return 1;
}
