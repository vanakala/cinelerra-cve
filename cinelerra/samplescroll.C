// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

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

SampleScroll::SampleScroll(int x, int y, int w)
 : BC_ScrollBar(x, y, SCROLL_HORIZ, w, 0, 0, 0)
{
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
	reposition_window(theme_global->mhscroll_x,
		theme_global->mhscroll_y,
		theme_global->mhscroll_w);
}

void SampleScroll::set_position(void)
{
	ptstime new_pos = master_edl->local_session->view_start_pts;

	if(mwindow_global->gui->canvas)
	{
		lock->lock("set_position");
		ptstime handle_dur = (theme_global->mcanvas_w -
			BC_ScrollBar::get_span(SCROLL_VERT)) *
			master_edl->local_session->zoom_time;

		if(master_edl->local_session->view_start_pts >
				master_edl->total_length() - handle_dur)
		{
			new_pos = master_edl->total_length() - handle_dur;
			if(new_pos < 0)
				new_pos = 0;

			master_edl->local_session->view_start_pts = new_pos;
		}
		update_length(round(master_edl->total_length() /
			master_edl->local_session->zoom_time),
			round(master_edl->local_session->view_start_pts /
			master_edl->local_session->zoom_time),
			round(handle_dur / master_edl->local_session->zoom_time));
		lock->unlock();
	}
}

int SampleScroll::handle_event()
{
	lock->lock("handle_event");
	master_edl->local_session->view_start_pts = get_value() *
		master_edl->local_session->zoom_time;
	mwindow_global->update_gui(WUPD_CANVINCR | WUPD_PATCHBAY | WUPD_TIMEBAR);
	lock->unlock();
	return 1;
}
