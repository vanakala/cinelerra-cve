
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

#include "bctitle.h"
#include "bcbutton.h"
#include "clip.h"
#include "cinelerra.h"
#include "edl.h"
#include "edlsession.h"
#include "language.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "plugin.h"
#include "track.h"
#include "transitionpopup.h"


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
	(float)0, 
	(float)100, 
	x,
	y,
	100)
{
	this->gui = gui;
}

int TransitionLengthText::handle_event()
{
	double result = atof(get_text());
	if(!EQUIV(result, gui->transition->get_length()))
	{
		gui->transition->set_length(result);
		if(gui->transition->track->data_type == TRACK_VIDEO) 
			mwindow_global->restart_brender();
		mwindow_global->sync_parameters(CHANGE_PARAMS);
		edlsession->default_transition_length = result;
		mwindow_global->gui->update(WUPD_CANVINCR);
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
	if(transition->plugin_server->uses_gui)
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
	if(mwindow_global->stop_composer())
		return 0;
	popup->transition->on = !get_checked();
	mwindow_global->restart_brender();
	mwindow_global->sync_parameters(CHANGE_EDL);
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
	popup->length_thread->start();
	return 1;
}
