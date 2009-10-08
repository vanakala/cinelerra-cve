
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
 : Thread(1, 0, 0)
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
// Not working in NPTL for some reason
//		Thread::cancel();
		Thread::join();
	}


	delete startup_lock;
}

void Tracking::create_objects()
{
//	start();
}

int Tracking::start_playback(double new_position)
{
	if(state != PLAYING)
	{
		last_position = new_position;
		state = PLAYING;
		draw();
		Thread::start();
		startup_lock->lock("Tracking::start_playback");
	}
	return 0;
}

int Tracking::stop_playback()
{
	if(state != DONE)
	{
// Stop loop
		state = DONE;
// Not working in NPTL for some reason
//		Thread::cancel();
		Thread::join();

// Final position is updated continuously during playback
// Get final position
		double position = get_tracking_position();
// Update cursor
		update_tracker(position);
	
		stop_meters();
		state = DONE;
	}
	return 0;
}

PlaybackEngine* Tracking::get_playback_engine()
{
	return mwindow->cwindow->playback_engine;
}

double Tracking::get_tracking_position()
{
	return get_playback_engine()->get_tracking_position();
}

int Tracking::get_pixel(double position)
{
	return (int64_t)((position - mwindow->edl->local_session->view_start) *
		mwindow->edl->session->sample_rate / 
		mwindow->edl->local_session->zoom_sample + 
		0.5);
}


void Tracking::update_meters(int64_t position)
{
	double output_levels[MAXCHANNELS];
	int do_audio = get_playback_engine()->get_output_levels(output_levels, position);

	if(do_audio)
	{
		module_levels.remove_all();
		get_playback_engine()->get_module_levels(&module_levels, position);

		mwindow->cwindow->gui->lock_window("Tracking::update_meters 1");
		mwindow->cwindow->gui->meters->update(output_levels);
		mwindow->cwindow->gui->unlock_window();

		mwindow->lwindow->gui->lock_window("Tracking::update_meters 2");
		mwindow->lwindow->gui->panel->update(output_levels);
		mwindow->lwindow->gui->unlock_window();

		mwindow->gui->lock_window("Tracking::update_meters 3");
		mwindow->gui->patchbay->update_meters(&module_levels);
		mwindow->gui->unlock_window();
	}
}

void Tracking::stop_meters()
{
	mwindow->cwindow->gui->lock_window("Tracking::stop_meters 1");
	mwindow->cwindow->gui->meters->stop_meters();
	mwindow->cwindow->gui->unlock_window();

	mwindow->gui->lock_window("Tracking::stop_meters 2");
	mwindow->gui->patchbay->stop_meters();
	mwindow->gui->unlock_window();

	mwindow->lwindow->gui->lock_window("Tracking::stop_meters 3");
	mwindow->lwindow->gui->panel->stop_meters();
	mwindow->lwindow->gui->unlock_window();
}




void Tracking::update_tracker(double position)
{
}

void Tracking::draw()
{
	gui->lock_window("Tracking::draw");
	if(!visible)
	{
		pixel = get_pixel(last_position);
	}

	gui->canvas->set_color(GREEN);
	gui->canvas->set_inverse();
	gui->canvas->draw_line(pixel, 0, pixel, gui->canvas->get_h());
	gui->canvas->set_opaque();
	gui->canvas->flash(pixel, 0, pixel + 1, gui->canvas->get_h());
	visible ^= 1;
	gui->unlock_window();
}


void Tracking::run()
{
	startup_lock->unlock();

	double position;
	while(state != DONE)
	{
		Thread::enable_cancel();
		timer.delay(1000 / TRACKING_RATE);
		Thread::disable_cancel();

		if(state != DONE)
		{

// can be stopped during wait
			if(get_playback_engine()->tracking_active)
			{
// Get position of cursor
				position = get_tracking_position();

// Update cursor
				update_tracker(position);
			}
		}
	}
}





