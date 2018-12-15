
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
#include "edl.h"
#include "edlsession.h"
#include "samplescroll.h"
#include "localsession.h"
#include "maincursor.h"
#include "mutex.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "patchbay.h"
#include "mainsession.h"
#include "mtimebar.h"
#include "theme.h"
#include "trackcanvas.h"
#include "tracks.h"

SampleScroll::SampleScroll(MWindow *mwindow, 
	int x,
	int y,
	int w)
 : BC_ScrollBar(x, 
	y,
	SCROLL_HORIZ, 
	w, 
	0, 
	0, 
	0)
{
	this->mwindow = mwindow;
	lock = new Mutex("SampleScroll");
}

SampleScroll::~SampleScroll()
{
	lock->lock("~SampleScroll");
	lock->unlock();
	delete lock;
}

void SampleScroll::resize_event(void)
{
	reposition_window(mwindow->theme->mhscroll_x,
		mwindow->theme->mhscroll_y, 
		mwindow->theme->mhscroll_w);
}

void SampleScroll::set_position(void)
{
	if(mwindow->gui->canvas)
	{
		lock->lock("set_position");
		int64_t length = round(master_edl->tracks->total_length() /
			master_edl->local_session->zoom_time);
		int64_t position = round(master_edl->local_session->view_start_pts /
			master_edl->local_session->zoom_time);
		int handle_size = mwindow->theme->mcanvas_w -
			BC_ScrollBar::get_span(SCROLL_VERT);

		if(position > length - handle_size)
		{
			position = length - handle_size;
			if(position < 0)
				position = 0;
			master_edl->local_session->view_start_pts =
				(ptstime)position * master_edl->local_session->zoom_time;
		}
		update_length(length, position, handle_size);
		lock->unlock();
	}
}

int SampleScroll::handle_event()
{
	lock->lock("handle_event");
	master_edl->local_session->view_start_pts = get_value() *
		master_edl->local_session->zoom_time;

	mwindow->gui->canvas->draw();
	mwindow->gui->cursor->draw(1);

	mwindow->gui->canvas->flash();
	mwindow->gui->patchbay->update();
	mwindow->gui->timebar->update();
	lock->unlock();
	return 1;
}
