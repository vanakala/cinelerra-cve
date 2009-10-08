
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

#include "clip.h"
#include "edit.h"
#include "edl.h"
#include "edlsession.h"
#include "language.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "plugin.h"
#include "transition.h"
#include "track.h"
#include "tracks.h"
#include "transitionpopup.h"


TransitionLengthThread::TransitionLengthThread(MWindow *mwindow, TransitionPopup *popup)
 : Thread()
{
	this->mwindow = mwindow;
	this->popup = popup;
}

TransitionLengthThread::~TransitionLengthThread()
{
}

void TransitionLengthThread::run()
{
	TransitionLengthDialog window(mwindow, popup->transition);
	window.create_objects();
	int result = window.run_window();
}






TransitionLengthDialog::TransitionLengthDialog(MWindow *mwindow, Transition *transition)
 : BC_Window(PROGRAM_NAME ": Transition length", 
				mwindow->gui->get_abs_cursor_x(1) - 150,
				mwindow->gui->get_abs_cursor_y(1) - 50,
				300, 
				100, 
				-1, 
				-1, 
				0,
				0, 
				1)
{
	this->mwindow = mwindow;
	this->transition = transition;
}

TransitionLengthDialog::~TransitionLengthDialog()
{
}

	
void TransitionLengthDialog::create_objects()
{
	add_subwindow(new BC_Title(10, 10, _("Seconds:")));
	text = new TransitionLengthText(mwindow, this, 100, 10);
	text->create_objects();
	add_subwindow(new BC_OKButton(this));
	show_window();
}

int TransitionLengthDialog::close_event()
{
	set_done(0);
	return 1;
}






TransitionLengthText::TransitionLengthText(MWindow *mwindow, 
	TransitionLengthDialog *gui,
	int x, 
	int y)
 : BC_TumbleTextBox(gui, 
 	(float)gui->transition->edit->track->from_units(gui->transition->length),
	(float)0, 
	(float)100, 
	x,
	y,
	100)
{
	this->mwindow = mwindow;
	this->gui = gui;
}

int TransitionLengthText::handle_event()
{
	double result = atof(get_text());
	if(!EQUIV(result, gui->transition->length))
	{
		gui->transition->length = gui->transition->track->to_units(result, 1);
		if(gui->transition->edit->track->data_type == TRACK_VIDEO) mwindow->restart_brender();
		mwindow->sync_parameters(CHANGE_PARAMS);
		mwindow->edl->session->default_transition_length = result;
		mwindow->gui->lock_window();
		mwindow->gui->update(0,
			1,
			0,
			0,
			0, 
			0,
			0);
		mwindow->gui->unlock_window();
	}
	
	return 1;
}











TransitionPopup::TransitionPopup(MWindow *mwindow, MWindowGUI *gui)
 : BC_PopupMenu(0, 
		0, 
		0, 
		"", 
		0)
{
	this->mwindow = mwindow;
	this->gui = gui;
}

TransitionPopup::~TransitionPopup()
{
//	delete dialog_thread;
}


void TransitionPopup::create_objects()
{
	length_thread = new TransitionLengthThread(mwindow, this);
//	add_item(attach = new TransitionPopupAttach(mwindow, this));
	add_item(show = new TransitionPopupShow(mwindow, this));
	add_item(on = new TransitionPopupOn(mwindow, this));
	add_item(length = new TransitionPopupLength(mwindow, this));
	add_item(detach = new TransitionPopupDetach(mwindow, this));
}

int TransitionPopup::update(Transition *transition)
{
	this->transition = transition;
	show->set_checked(transition->show);
	on->set_checked(transition->on);
	char len_text[50];
	sprintf(len_text, _("Length: %2.2f sec"), transition->track->from_units(transition->length));
	length->set_text(len_text);
	return 0;
}





TransitionPopupAttach::TransitionPopupAttach(MWindow *mwindow, TransitionPopup *popup)
 : BC_MenuItem(_("Attach..."))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

TransitionPopupAttach::~TransitionPopupAttach()
{
}

int TransitionPopupAttach::handle_event()
{
//	popup->dialog_thread->start();
}







TransitionPopupDetach::TransitionPopupDetach(MWindow *mwindow, TransitionPopup *popup)
 : BC_MenuItem(_("Detach"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

TransitionPopupDetach::~TransitionPopupDetach()
{
}

int TransitionPopupDetach::handle_event()
{
	mwindow->detach_transition(popup->transition);
	return 1;
}


TransitionPopupOn::TransitionPopupOn(MWindow *mwindow, TransitionPopup *popup)
 : BC_MenuItem(_("On"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

TransitionPopupOn::~TransitionPopupOn()
{
}

int TransitionPopupOn::handle_event()
{
	popup->transition->on = !get_checked();
	mwindow->sync_parameters(CHANGE_EDL);
	return 1;
}






TransitionPopupShow::TransitionPopupShow(MWindow *mwindow, TransitionPopup *popup)
 : BC_MenuItem(_("Show"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

TransitionPopupShow::~TransitionPopupShow()
{
}

int TransitionPopupShow::handle_event()
{
	mwindow->show_plugin(popup->transition);
	return 1;
}








TransitionPopupLength::TransitionPopupLength(MWindow *mwindow, TransitionPopup *popup)
 : BC_MenuItem(_("Length"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

TransitionPopupLength::~TransitionPopupLength()
{
}

int TransitionPopupLength::handle_event()
{
	popup->length_thread->start();
	return 1;
}

