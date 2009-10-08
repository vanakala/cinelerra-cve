
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
#include "tracks.h"
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
}

EditPopup::~EditPopup()
{
}

void EditPopup::create_objects()
{
	add_item(new EditAttachEffect(mwindow, this));
	add_item(new EditMoveTrackUp(mwindow, this));
	add_item(new EditMoveTrackDown(mwindow, this));
	add_item(new EditPopupDeleteTrack(mwindow, this));
	add_item(new EditPopupAddTrack(mwindow, this));
//	add_item(new EditPopupTitle(mwindow, this));
	resize_option = 0;
}

int EditPopup::update(Track *track, Edit *edit)
{
	this->edit = edit;
	this->track = track;

	if(track->data_type == TRACK_VIDEO && !resize_option)
	{
		add_item(resize_option = new EditPopupResize(mwindow, this));
		add_item(matchsize_option = new EditPopupMatchSize(mwindow, this));
	}
	else
	if(track->data_type == TRACK_AUDIO && resize_option)
	{
		remove_item(resize_option);
		remove_item(matchsize_option);
		resize_option = 0;
		matchsize_option = 0;
	}
	return 0;
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
		PROGRAM_NAME ": Attach Effect");
	return 1;
}


EditMoveTrackUp::EditMoveTrackUp(MWindow *mwindow, EditPopup *popup)
 : BC_MenuItem(_("Move up"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}
EditMoveTrackUp::~EditMoveTrackUp()
{
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
EditMoveTrackDown::~EditMoveTrackDown()
{
}
int EditMoveTrackDown::handle_event()
{
	mwindow->move_track_down(popup->track);
	return 1;
}




EditPopupResize::EditPopupResize(MWindow *mwindow, EditPopup *popup)
 : BC_MenuItem(_("Resize track..."))
{
	this->mwindow = mwindow;
	this->popup = popup;
	dialog_thread = new ResizeTrackThread(mwindow, 
		popup->track->tracks->number_of(popup->track));
}
EditPopupResize::~EditPopupResize()
{
	delete dialog_thread;
}

int EditPopupResize::handle_event()
{
	dialog_thread->start_window(popup->track, popup->track->tracks->number_of(popup->track));
	return 1;
}






EditPopupMatchSize::EditPopupMatchSize(MWindow *mwindow, EditPopup *popup)
 : BC_MenuItem(_("Match output size"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}
EditPopupMatchSize::~EditPopupMatchSize()
{
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
	if(popup->track->data_type == TRACK_AUDIO)
		mwindow->add_audio_track_entry(1, popup->track);
	else
		mwindow->add_video_track_entry(popup->track);
	return 1;
}






EditPopupTitle::EditPopupTitle(MWindow *mwindow, EditPopup *popup)
 : BC_MenuItem(_("User title..."))
{
	this->mwindow = mwindow;
	this->popup = popup;
	window = 0;
}

EditPopupTitle::~EditPopupTitle()
{
	delete popup;
}

int EditPopupTitle::handle_event()
{
	int result;

	Track *trc = mwindow->session->track_highlighted;

	if (trc && trc->record)
	{
		Edit *edt = mwindow->session->edit_highlighted;
		if(!edt) return 1;

		window = new EditPopupTitleWindow (mwindow, popup);
		window->create_objects();
		result = window->run_window();


		if(!result && edt)
		{
			strcpy(edt->user_title, window->title_text->get_text());
		}

		delete window;
		window = 0;
	}

	return 1;
}


EditPopupTitleWindow::EditPopupTitleWindow (MWindow *mwindow, EditPopup *popup)
 : BC_Window (PROGRAM_NAME ": Set edit title",
	mwindow->gui->get_abs_cursor_x(0) - 400 / 2,
	mwindow->gui->get_abs_cursor_y(0) - 500 / 2,
	300,
	100,
	300,
	100,
	0,
	0,
	1)
{
	this->mwindow = mwindow;
	this->popup = popup;
	this->edt = this->mwindow->session->edit_highlighted;
	if(this->edt)
	{
		strcpy(new_text, this->edt->user_title);
	}
}

EditPopupTitleWindow::~EditPopupTitleWindow()
{
}

int EditPopupTitleWindow::close_event()
{
	set_done(1);
	return 1;
}

int EditPopupTitleWindow::create_objects()
{
	int x = 5;
	int y = 10;

	add_subwindow (new BC_Title (x, y, _("User title")));
	add_subwindow (title_text = new EditPopupTitleText (this,
		mwindow, x, y + 20));
	add_tool(new BC_OKButton(this));
	add_tool(new BC_CancelButton(this));


	show_window();
	flush();
	return 0;
}


EditPopupTitleText::EditPopupTitleText (EditPopupTitleWindow *window, 
	MWindow *mwindow, int x, int y)
 : BC_TextBox(x, y, 250, 1, (char*)(window->edt ? window->edt->user_title : ""))
{
	this->window = window;
	this->mwindow = mwindow;
}

EditPopupTitleText::~EditPopupTitleText() 
{ 
}
 
int EditPopupTitleText::handle_event()
{
	return 1;
}







