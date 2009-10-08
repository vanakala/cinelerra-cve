
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

#include "bcsignals.h"
#include "ctimebar.h"
#include "cwindow.h"
#include "cwindowgui.h"
#include "edl.h"
#include "localsession.h"
#include "maincursor.h"
#include "mbuttons.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "theme.h"


CTimeBar::CTimeBar(MWindow *mwindow, 
		CWindowGUI *gui,
		int x,
		int y,
		int w, 
		int h)
 : TimeBar(mwindow, 
		gui,
		x, 
		y,
		w,
		h)
{
	this->mwindow = mwindow;
	this->gui = gui;
}

int CTimeBar::resize_event()
{
	reposition_window(mwindow->theme->ctimebar_x,
		mwindow->theme->ctimebar_y,
		mwindow->theme->ctimebar_w,
		mwindow->theme->ctimebar_h);
	update();
	return 1;
}


EDL* CTimeBar::get_edl()
{
	return mwindow->edl;
}

void CTimeBar::draw_time()
{
	get_edl_length();
	draw_range();
}


void CTimeBar::update_preview()
{
	gui->slider->set_position();
}


void CTimeBar::select_label(double position)
{
	EDL *edl = mwindow->edl;

	gui->unlock_window();
	mwindow->gui->mbuttons->transport->handle_transport(STOP, 1, 0, 0);
	gui->lock_window();

	position = mwindow->edl->align_to_frame(position, 1);

	if(shift_down())
	{
		if(position > edl->local_session->get_selectionend(1) / 2 + 
			edl->local_session->get_selectionstart(1) / 2)
		{
		
			edl->local_session->set_selectionend(position);
		}
		else
		{
			edl->local_session->set_selectionstart(position);
		}
	}
	else
	{
		edl->local_session->set_selectionstart(position);
		edl->local_session->set_selectionend(position);
	}

// Que the CWindow
	mwindow->cwindow->update(1, 0, 0, 0, 1);

//printf("CTimeBar::select_label 1\n");

	mwindow->gui->lock_window();
	mwindow->gui->cursor->hide(0);
	mwindow->gui->cursor->draw(1);
	mwindow->gui->update(0,
		1,      // 1 for incremental drawing.  2 for full refresh
		1,
		0,
		1, 
		1,
		0);
	mwindow->gui->unlock_window();
	mwindow->update_plugin_guis();
//printf("CTimeBar::select_label 2\n");
}





