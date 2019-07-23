
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

#include "edit.h"
#include "editpopup.h"
#include "language.h"
#include "mainsession.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "plugindialog.h"
#include "resizetrackthread.h"
#include "track.h"
#include "trackcanvas.h"


#include <string.h>

EditPopup::EditPopup(MWindow *mwindow, MWindowGUI *gui)
 : BC_PopupMenu(0, 
		0, 
		0, 
		"", 
		0)
{
	this->mwindow = mwindow;
	this->gui = gui;
	add_item(new EditAttachEffect(mwindow, this));
	add_item(new EditMoveTrackUp(mwindow, this));
	add_item(new EditMoveTrackDown(mwindow, this));
	add_item(new EditPopupDeleteTrack(mwindow, this));
	add_item(new EditPopupAddTrack(mwindow, this));
	resize_option = new EditPopupResize(this);
	matchsize_option = new EditPopupMatchSize(mwindow, this);
}

EditPopup::~EditPopup()
{
	if(track->data_type == TRACK_AUDIO)
	{
// These are deleted by window if they are subwindows
		delete resize_option;
		delete matchsize_option;
	}
}

void EditPopup::update(Track *track, Edit *edit)
{
	this->edit = edit;
	this->track = track;

	if(track->data_type == TRACK_VIDEO)
	{
		add_item(resize_option);
		add_item(matchsize_option);
	}
	else
	if(track->data_type == TRACK_AUDIO)
	{
		remove_item(resize_option);
		remove_item(matchsize_option);
	}
}


EditAttachEffect::EditAttachEffect(MWindow *mwindow, EditPopup *popup)
 : BC_MenuItem(_("Attach effect..."))
{
	this->mwindow = mwindow;
	this->popup = popup;
	dialog_thread = new PluginDialogThread(mwindow);
}

EditAttachEffect::~EditAttachEffect()
{
	delete dialog_thread;
}

int EditAttachEffect::handle_event()
{
	dialog_thread->start_window(popup->track,
		0, 
		MWindow::create_title(N_("Attach Effect")));
	return 1;
}


EditMoveTrackUp::EditMoveTrackUp(MWindow *mwindow, EditPopup *popup)
 : BC_MenuItem(_("Move up"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

int EditMoveTrackUp::handle_event()
{
	mwindow->move_track_up(popup->track);
	return 1;
}


EditMoveTrackDown::EditMoveTrackDown(MWindow *mwindow, EditPopup *popup)
 : BC_MenuItem(_("Move down"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

int EditMoveTrackDown::handle_event()
{
	mwindow->move_track_down(popup->track);
	return 1;
}


EditPopupResize::EditPopupResize(EditPopup *popup)
 : BC_MenuItem(_("Resize track..."))
{
	this->popup = popup;
	dialog_thread = new ResizeTrackThread();
}

int EditPopupResize::handle_event()
{
	if(dialog_thread->running())
	{
		if(dialog_thread->window)
			dialog_thread->window->raise_window();
	}
	else
		dialog_thread->start_window(popup->track);
	return 1;
}


EditPopupMatchSize::EditPopupMatchSize(MWindow *mwindow, EditPopup *popup)
 : BC_MenuItem(_("Match output size"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

int EditPopupMatchSize::handle_event()
{
	mwindow->match_output_size(popup->track);
	return 1;
}


EditPopupDeleteTrack::EditPopupDeleteTrack(MWindow *mwindow, EditPopup *popup)
 : BC_MenuItem(_("Delete track"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

int EditPopupDeleteTrack::handle_event()
{
	mwindow->delete_track(popup->track);
	return 1;
}


EditPopupAddTrack::EditPopupAddTrack(MWindow *mwindow, EditPopup *popup)
 : BC_MenuItem(_("Add track"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

int EditPopupAddTrack::handle_event()
{
	mwindow->add_track(popup->track->data_type, 1, popup->track);
	return 1;
}
