
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

#ifndef CPLAYBACKCURSOR_H
#define CPLAYBACKCURSOR_H

#include "cwindow.inc"
#include "mwindow.inc"
#include "tracking.h"
#include "playbackengine.inc"

class CTracking : public Tracking
{
public:
	CTracking(MWindow *mwindow, CWindow *cwindow);
	~CTracking();

	PlaybackEngine* get_playback_engine();
	void update_tracker(double position);
// Move Track Canvas left or right to track playback
	int update_scroll(double position);

// Move sliders and insertion point to track playback
	int start_playback(double new_position);
	int stop_playback();
	void draw();

	CWindow *cwindow;
};

#endif
