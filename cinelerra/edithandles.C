
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

#include "cursors.h"
#include "edithandles.h"
#include "edithandles.inc"
#include "edit.h"
#include "edits.h"
#include "edl.h"
#include "mwindow.h"
#include "theme.h"
#include "track.h"
#include "tracks.h"
#include "trackcanvas.h"



EditHandle::EditHandle(MWindow *mwindow, 
		TrackCanvas *trackcanvas, 
		Edit *edit, 
		int side, 
		int x, 
		int y)
 : CanvasTool(mwindow,
 	trackcanvas,
	edit, 
	x,
	y, 
	side == EDIT_IN ? mwindow->theme->edithandlein_data : mwindow->theme->edithandleout_data)
{
	this->side = side;
}

EditHandle::~EditHandle()
{
}

int EditHandle::handle_event()
{
	return 0;
}





EditHandleIn::EditHandleIn(MWindow *mwindow, 
	TrackCanvas *trackcanvas,
	Edit *edit,
	int x,
	int y)
 : EditHandle(mwindow, 
		trackcanvas, 
		edit, 
		EDIT_IN, 
		x, 
		y)
{
}

EditHandleIn::~EditHandleIn()
{
}


int EditHandleIn::handle_event()
{
	return 1;
}








EditHandleOut::EditHandleOut(MWindow *mwindow, 
	TrackCanvas *trackcanvas,
	Edit *edit,
	int x,
	int y)
 : EditHandle(mwindow, 
		trackcanvas, 
		edit, 
		EDIT_OUT, 
		x, 
		y)
{
	this->mwindow = mwindow;
	this->trackcanvas = trackcanvas;
	this->edit = edit;
	visible = 1;
}

EditHandleOut::~EditHandleOut()
{
}


int EditHandleOut::handle_event()
{
	return 1;
}










EditHandles::EditHandles(MWindow *mwindow, 
		TrackCanvas *trackcanvas)
 : CanvasTools(mwindow, trackcanvas)
{
}

EditHandles::~EditHandles()
{
}

void EditHandles::update()
{
	decrease_visible();

	for(Track *current = mwindow->edl->tracks->first;
		current;
		current = NEXT)
	{
		for(Edit *edit = current->edits->first; edit; edit = edit->next)
		{
// Test in point
			int64_t handle_x, handle_y, handle_w, handle_h;
			trackcanvas->get_handle_coords(edit, handle_x, handle_y, handle_w, handle_h, EDIT_IN);

			if(visible(handle_x, handle_y, handle_w, handle_h))
			{
				int exists = 0;
				
				for(int i = 0; i < total; i++)
				{
					EditHandle *handle = (EditHandle*)values[i];
					if(handle->edit->id == edit->id && handle->side == EDIT_IN)
					{
						handle->reposition_window(handle_x, handle_y);
						handle->raise_window();
						handle->draw_face();
						handle->visible = 1;
						exists = 1;
						break;
					}
				}
				
				if(!exists)
				{
					EditHandle *handle = new EditHandle(mwindow, 
						trackcanvas, 
						edit, 
						EDIT_IN, 
						handle_x, 
						handle_y);
					trackcanvas->add_subwindow(handle);
					handle->set_cursor(ARROW_CURSOR);
					append(handle);
				}
			}


// Test out point
			trackcanvas->get_handle_coords(edit, handle_x, handle_y, handle_w, handle_h, EDIT_OUT);

			if(visible(handle_x, handle_y, handle_w, handle_h))
			{
				int exists = 0;
				
				for(int i = 0; i < total; i++)
				{
					EditHandle *handle = (EditHandle*)values[i];
					if(handle->edit->id == edit->id && handle->side == EDIT_OUT)
					{
						handle->reposition_window(handle_x, handle_y);
						handle->raise_window();
						handle->draw_face();
						handle->visible = 1;
						exists = 1;
						break;
					}
				}
				
				if(!exists)
				{
					EditHandle *handle = new EditHandle(mwindow, 
						trackcanvas, 
						edit, 
						EDIT_OUT, 
						handle_x, 
						handle_y);
					trackcanvas->add_subwindow(handle);
					handle->set_cursor(ARROW_CURSOR);
					append(handle);
				}
			}
		}
	}

	delete_invisible();
}
