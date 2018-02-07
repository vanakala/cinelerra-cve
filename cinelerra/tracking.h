
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

#ifndef TRACKING_H
#define TRACKING_H

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "condition.inc"
#include "datatype.h"
#include "mwindow.inc"
#include "mwindowgui.inc"
#include "playbackengine.inc"
#include "thread.h"
#include "bctimer.h"

class Tracking : public Thread
{
public:
	Tracking(MWindow *mwindow);
	virtual ~Tracking();

	virtual void start_playback(ptstime new_position);
	virtual void stop_playback();

// Called by the tracker to get the current position
	virtual PlaybackEngine* get_playback_engine() = 0;
	ptstime get_tracking_position();
// Update position displayed
	virtual void update_tracker(ptstime position) {};
// Update meters
	virtual void update_meters(ptstime pts) {};
	virtual void stop_meters() {};
	virtual void set_delays(float over_delay, float peak_delay) {};

	void run();

	int state;
	int view_follows_playback;
// Delay until startup
	Condition *startup_lock;
	MWindow *mwindow;
	MWindowGUI *gui;
	ptstime last_position;
	int follow_loop;
	int reverse;
	int double_speed;
	Timer timer;
// Pixel of last drawn cursor
	int pixel;
// Cursor is visible
	int visible;
protected:
	int tracking_rate;
};

#endif
