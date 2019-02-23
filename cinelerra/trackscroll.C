
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
#include "edlsession.h"
#include "localsession.h"
#include "maincursor.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "patchbay.h"
#include "mainsession.h"
#include "theme.h"
#include "trackcanvas.h"
#include "tracks.h"
#include "trackscroll.h"

TrackScroll::TrackScroll(MWindow *mwindow, int x, int y, int h)
 : BC_ScrollBar(x, 
	y,
	SCROLL_VERT, 
	h, 
	0, 
	0, 
	0)
{
	this->mwindow = mwindow;
}

void TrackScroll::resize_event()
{
	reposition_window(mwindow->theme->mvscroll_x, 
		mwindow->theme->mvscroll_y, 
		mwindow->theme->mvscroll_h);
}

void TrackScroll::set_position(int handle_length)
{
	if(mwindow->gui->canvas)
	{
		int64_t length = master_edl->get_tracks_height(mwindow->theme);
		int64_t position = master_edl->local_session->track_start;

		if(position > length - handle_length)
		{
			position = length - handle_length;
			if(position < 0)
				position = 0;
			master_edl->local_session->track_start = position;
		}
		update_length(length, position, handle_length);
	}
}

int TrackScroll::handle_event()
{
	master_edl->local_session->track_start = get_value();
	master_edl->tracks->update_y_pixels(mwindow->theme);
	mwindow->gui->canvas->draw();
	mwindow->gui->cursor->draw(1);
	mwindow->gui->patchbay->update();
	mwindow->gui->canvas->flash(1);
	return 1;
}
