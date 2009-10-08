
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

#include "edl.h"
#include "localsession.h"
#include "mwindow.h"
#include "theme.h"
#include "vtimebar.h"
#include "vwindow.h"
#include "vwindowgui.h"


VTimeBar::VTimeBar(MWindow *mwindow, 
		VWindowGUI *gui,
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

int VTimeBar::resize_event()
{
	reposition_window(mwindow->theme->vtimebar_x,
		mwindow->theme->vtimebar_y,
		mwindow->theme->vtimebar_w,
		mwindow->theme->vtimebar_h);
	update();
	return 1;
}

EDL* VTimeBar::get_edl()
{
	return gui->vwindow->get_edl();
}

void VTimeBar::draw_time()
{
	draw_range();
}

void VTimeBar::update_preview()
{
	gui->slider->set_position();
}


void VTimeBar::select_label(double position)
{
	EDL *edl = get_edl();

	if(edl)
	{
		unlock_window();
		gui->transport->handle_transport(STOP, 1, 0, 0);
		lock_window();

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
		mwindow->vwindow->update_position(CHANGE_NONE, 
			0, 
			1);
		update();
	}
}



