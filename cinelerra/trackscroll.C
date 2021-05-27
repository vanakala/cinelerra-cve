// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bcsignals.h"
#include "cinelerra.h"
#include "edl.h"
#include "localsession.h"
#include "maincursor.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "theme.h"
#include "trackcanvas.h"
#include "tracks.h"
#include "trackscroll.h"

TrackScroll::TrackScroll()
 : BC_ScrollBar(theme_global->mvscroll_x,
	theme_global->mvscroll_y,
	SCROLL_VERT,
	theme_global->mvscroll_h,
	0,
	0,
	0)
{
}

void TrackScroll::resize_event()
{
	reposition_window(theme_global->mvscroll_x,
		theme_global->mvscroll_y,
		theme_global->mvscroll_h);
}

void TrackScroll::set_position(int handle_length)
{
	if(mwindow_global->gui->canvas)
	{
		int length = master_edl->get_tracks_height(theme_global);
		int position = master_edl->local_session->track_start;

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
	master_edl->tracks->update_y_pixels(theme_global);
	mwindow_global->update_gui(WUPD_CANVINCR | WUPD_PATCHBAY);
	return 1;
}
