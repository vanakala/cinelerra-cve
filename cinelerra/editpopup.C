#include "edit.h"
#include "editpopup.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "plugindialog.h"
#include "resizetrackthread.h"
#include "track.h"
#include "tracks.h"
#include "trackcanvas.h"

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
	resize_option = 0;
}

int EditPopup::update(Track *track, Edit *edit)
{
//	this->edit = edit;
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
 : BC_MenuItem("Attach effect...")
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
 : BC_MenuItem("Move up")
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
 : BC_MenuItem("Move down")
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
 : BC_MenuItem("Resize track...")
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
 : BC_MenuItem("Match output size")
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
 : BC_MenuItem("Delete track")
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
 : BC_MenuItem("Add track")
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














