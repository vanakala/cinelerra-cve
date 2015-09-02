
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

#include "arender.h"
#include "bcsignals.h"
#include "condition.h"
#include "cplayback.h"
#include "cwindow.h"
#include "cwindowgui.h"
#include "edl.h"
#include "edlsession.h"
#include "levelwindow.h"
#include "levelwindowgui.h"
#include "localsession.h"
#include "mainclock.h"
#include "meterpanel.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "tracking.h"
#include "patchbay.h"
#include "playbackengine.h"
#include "renderengine.h"
#include "mainsession.h"
#include "trackcanvas.h"



// States
#define PLAYING 0
#define DONE 1



Tracking::Tracking(MWindow *mwindow)
 : Thread(THREAD_SYNCHRONOUS)
{
	this->gui = mwindow->gui;
	this->mwindow = mwindow; 
	follow_loop = 0; 
	visible = 0;
	pixel = 0;
	state = DONE;
	startup_lock = new Condition(0, "Tracking::startup_lock");
}

Tracking::~Tracking()
{
	if(state == PLAYING)
	{
// Stop loop
		state = DONE;
		Thread::join();
	}
	delete startup_lock;
}

void Tracking::start_playback(ptstime new_position)
{
// Calculate tracking rate
	tracking_rate = mwindow->edl->session->frame_rate;
	if(tracking_rate <= 0)
		tracking_rate = TRACKING_RATE_DEFAULT;
	while(tracking_rate > TRACKING_RATE_MAX)
		tracking_rate /= 2;
	set_delays(mwindow->edl->session->meter_over_delay,
		mwindow->edl->session->meter_peak_delay);
	if(state != PLAYING)
	{
		last_position = new_position;
		state = PLAYING;
		Thread::start();
		startup_lock->lock("Tracking::start_playback");
	}
}

void Tracking::stop_playback()
{
	if(state != DONE)
	{
// Stop loop
		state = DONE;
		Thread::join();

// Final position is updated continuously during playback
// Update cursor
		update_tracker(get_tracking_position());

		stop_meters();
		state = DONE;
	}
}

PlaybackEngine* Tracking::get_playback_engine()
{
	return mwindow->cwindow->playback_engine;
}

ptstime Tracking::get_tracking_position()
{
	return get_playback_engine()->get_tracking_position();
}

void Tracking::update_meters(ptstime pts)
{
	double output_levels[MAXCHANNELS];
	double module_levels[MAXCHANNELS];

	if(get_playback_engine()->get_output_levels(output_levels, pts))
	{
		int n = get_playback_engine()->get_module_levels(module_levels, pts);

		mwindow->cwindow->gui->meters->update(output_levels);
		mwindow->lwindow->gui->panel->update(output_levels);
		mwindow->gui->patchbay->update_meters(module_levels, n);
	}
}

void Tracking::stop_meters()
{
	mwindow->cwindow->gui->meters->stop_meters();
	mwindow->gui->patchbay->stop_meters();
	mwindow->lwindow->gui->panel->stop_meters();
}

void Tracking::set_delays(float over_delay, float peak_delay)
{
	int over = over_delay * tracking_rate;
	int peak = peak_delay * tracking_rate;

	mwindow->cwindow->gui->meters->set_delays(over, peak);
	mwindow->gui->patchbay->set_delays(over, peak);
	mwindow->lwindow->gui->panel->set_delays(over, peak);
}

void Tracking::run()
{
	startup_lock->unlock();

	int delay = 1000 / tracking_rate;

	while(state != DONE)
	{
		Thread::enable_cancel();
		timer.delay(delay);
		Thread::disable_cancel();

		if(state != DONE)
		{
// can be stopped during wait
			if(get_playback_engine()->tracking_active)
				update_tracker(get_tracking_position());
		}
	}
}
