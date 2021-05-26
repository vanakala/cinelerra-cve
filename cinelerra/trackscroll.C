// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bcsignals.h"
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
	mwindow->update_gui(WUPD_CANVINCR | WUPD_PATCHBAY);
	return 1;
}
