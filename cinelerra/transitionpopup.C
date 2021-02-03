// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bctitle.h"
#include "bcbutton.h"
#include "clip.h"
#include "cinelerra.h"
#include "edl.h"
#include "edlsession.h"
#include "language.h"
#include "mwindow.h"
#include "plugin.h"
#include "track.h"
#include "transitionpopup.h"
#include "mainundo.h"


TransitionLengthThread::TransitionLengthThread(TransitionPopup *popup)
 : Thread()
{
	this->popup = popup;
}

void TransitionLengthThread::run()
{
	int cx, cy;

	mwindow_global->get_abs_cursor_pos(&cx, &cy);
	TransitionLengthDialog window(popup->transition, cx, cy);
	window.run_window();
}


TransitionLengthDialog::TransitionLengthDialog(Plugin *transition,
	int absx, int absy)
 : BC_Window(MWindow::create_title(N_("Transition length")),
		absx - 150,
		absy - 50,
		300,
		100,
		-1,
		-1,
		0,
		0,
		1)
{
	this->transition = transition;
	set_icon(mwindow_global->get_window_icon());
	add_subwindow(new BC_Title(10, 10, _("Seconds:")));
	text = new TransitionLengthText(this, 100, 10);
	add_subwindow(new BC_OKButton(this));
	show_window();
}

TransitionLengthText::TransitionLengthText(TransitionLengthDialog *gui,
	int x,
	int y)
 : BC_TumbleTextBox(gui, 
	gui->transition->get_length(),
	0.0,
	100.0,
	x,
	y,
	100)
{
	this->gui = gui;
	set_increment(0.1);
}

int TransitionLengthText::handle_event()
{
	double result = atof(get_text());
	if(!EQUIV(result, gui->transition->get_length()))
	{
		gui->transition->set_length(result);
		mwindow_global->sync_parameters(
			gui->transition->track->data_type == TRACK_VIDEO);
		edlsession->default_transition_length = result;
		mwindow_global->undo->update_undo(_("transition"), LOAD_EDITS);
		mwindow_global->update_gui(WUPD_CANVINCR);
	}
	return 1;
}


TransitionPopup::TransitionPopup()
 : BC_PopupMenu(0, 
		0, 
		0, 
		"", 
		0)
{
	length_thread = new TransitionLengthThread(this);
	show = new TransitionPopupShow(this);
	add_item(on = new TransitionPopupOn(this));
	add_item(length = new TransitionPopupLength(this));
	add_item(detach = new TransitionPopupDetach(this));
	has_gui = 0;
}

TransitionPopup::~TransitionPopup()
{
	delete length_thread;
	if(!has_gui)
		delete show;
}

void TransitionPopup::update(Plugin *transition)
{
	this->transition = transition;
	if(transition->plugin_server && transition->plugin_server->uses_gui)
	{
		if(!has_gui)
		{
			add_item(show);
			has_gui = 1;
		}
	}
	else
	{
		remove_item(show);
		has_gui = 0;
	}
	show->set_checked(transition->show);
	on->set_checked(transition->on);
	char len_text[50];
	sprintf(len_text, _("Length: %2.2f sec"), transition->get_length());
	length->set_text(len_text);
}


TransitionPopupDetach::TransitionPopupDetach(TransitionPopup *popup)
 : BC_MenuItem(_("Detach"))
{
	this->popup = popup;
}

int TransitionPopupDetach::handle_event()
{
	mwindow_global->detach_transition(popup->transition);
	return 1;
}


TransitionPopupOn::TransitionPopupOn(TransitionPopup *popup)
 : BC_MenuItem(_("On"))
{
	this->popup = popup;
}

int TransitionPopupOn::handle_event()
{
	if(!popup->transition->plugin_server)
	{
		popup->transition->on = 0;
		return 0;
	}
	if(mwindow_global->stop_composer())
		return 0;
	popup->transition->on = !get_checked();
	mwindow_global->sync_parameters();
	return 1;
}


TransitionPopupShow::TransitionPopupShow(TransitionPopup *popup)
 : BC_MenuItem(_("Parameters"))
{
	this->popup = popup;
}

int TransitionPopupShow::handle_event()
{
	mwindow_global->show_plugin(popup->transition);
	return 1;
}


TransitionPopupLength::TransitionPopupLength(TransitionPopup *popup)
 : BC_MenuItem(_("Length"))
{
	this->popup = popup;
}

int TransitionPopupLength::handle_event()
{
	if(mwindow_global->stop_composer())
		return 0;
	popup->length_thread->start();
	return 1;
}
