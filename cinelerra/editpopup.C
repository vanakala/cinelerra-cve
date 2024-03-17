// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "editpopup.h"
#include "language.h"
#include "mwindow.h"
#include "plugindialog.h"
#include "resizetrackthread.h"
#include "track.h"

#include <string.h>

EditPopup::EditPopup()
 : BC_PopupMenu(0, 0, 0, "", 0)
{
	add_item(new EditAttachEffect(this));
	add_item(new EditMoveTrackUp(this));
	add_item(new EditMoveTrackDown(this));
	add_item(new EditPopupDeleteTrack(this));
	add_item(new EditPopupAddTrack(this));
	resize_option = new EditPopupResize(this);
	matchsize_option = new EditPopupMatchSize(this);
	pastekeyframe = new EditPopupPasteKeyFrame(this);
	assetsize = new EditPopupMatchAssetSize(this);
	have_video = 0;
	have_keyframe = 0;
}

EditPopup::~EditPopup()
{
	if(!have_video)
	{
// These are deleted by window if they are subwindows
		delete resize_option;
		delete matchsize_option;
		delete assetsize;
	}
	if(have_keyframe)
		delete pastekeyframe;
}

void EditPopup::update(Track *track)
{
	this->track = track;

	if(track->data_type == TRACK_VIDEO)
	{
		if(!have_video)
		{
			add_item(resize_option);
			add_item(matchsize_option);
			add_item(assetsize);
			have_video = 1;
		}
	}
	else if(track->data_type == TRACK_AUDIO)
	{
		remove_item(resize_option);
		remove_item(matchsize_option);
		remove_item(assetsize);
		have_video = 0;
	}
	if(mwindow_global->can_paste_keyframe(track, 0))
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
}

EditAttachEffect::EditAttachEffect(EditPopup *popup)
 : BC_MenuItem(_("Attach effect..."))
{
	this->popup = popup;
	dialog_thread = new PluginDialogThread();
}

EditAttachEffect::~EditAttachEffect()
{
	delete dialog_thread;
}

int EditAttachEffect::handle_event()
{
	dialog_thread->start_window(popup->track);
	return 1;
}


EditMoveTrackUp::EditMoveTrackUp(EditPopup *popup)
 : BC_MenuItem(_("Move up"))
{
	this->popup = popup;
}

int EditMoveTrackUp::handle_event()
{
	mwindow_global->move_track_up(popup->track);
	return 1;
}


EditMoveTrackDown::EditMoveTrackDown(EditPopup *popup)
 : BC_MenuItem(_("Move down"))
{
	this->popup = popup;
}

int EditMoveTrackDown::handle_event()
{
	mwindow_global->move_track_down(popup->track);
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


EditPopupMatchSize::EditPopupMatchSize(EditPopup *popup)
 : BC_MenuItem(_("Match output size"))
{
	this->popup = popup;
}

int EditPopupMatchSize::handle_event()
{
	mwindow_global->match_output_size(popup->track);
	return 1;
}


EditPopupDeleteTrack::EditPopupDeleteTrack(EditPopup *popup)
 : BC_MenuItem(_("Delete track"))
{
	this->popup = popup;
}

int EditPopupDeleteTrack::handle_event()
{
	mwindow_global->delete_track(popup->track);
	return 1;
}


EditPopupAddTrack::EditPopupAddTrack(EditPopup *popup)
 : BC_MenuItem(_("Add track"))
{
	this->popup = popup;
}

int EditPopupAddTrack::handle_event()
{
	mwindow_global->add_track(popup->track->data_type, 1, popup->track);
	return 1;
}

EditPopupPasteKeyFrame::EditPopupPasteKeyFrame(EditPopup *popup)
 : BC_MenuItem(_("Paste keyframe"))
{
	this->popup = popup;
}

int EditPopupPasteKeyFrame::handle_event()
{
	mwindow_global->paste_keyframe(popup->track, 0);
	return 1;
}

EditPopupMatchAssetSize::EditPopupMatchAssetSize(EditPopup *popup)
 : BC_MenuItem(_("Match input size"))
{
	this->popup = popup;
}

int EditPopupMatchAssetSize::handle_event()
{
	mwindow_global->match_asset_size(popup->track);
	return 1;
}
