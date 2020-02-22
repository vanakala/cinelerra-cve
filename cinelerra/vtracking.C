
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

#include "asset.h"
#include "bcsignals.h"
#include "edl.h"
#include "localsession.h"
#include "mainclock.h"
#include "meterpanel.h"
#include "mwindow.h"
#include "renderengine.h"
#include "mainclock.h"
#include "vplayback.h"
#include "vtimebar.h"
#include "vtracking.h"
#include "vwindow.h"
#include "vwindowgui.h"


VTracking::VTracking(MWindow *mwindow, VWindow *vwindow)
 : Tracking(mwindow)
{
	this->vwindow = vwindow;
}

PlaybackEngine* VTracking::get_playback_engine()
{
	return vwindow->playback_engine;
}

void VTracking::update_tracker(ptstime position)
{
	vwindow_edl->local_session->set_selection(position);
	vwindow->gui->slider->update(position);

	vwindow->gui->clock->update(position);
	vwindow->gui->timebar->update();

	update_meters(position);
}

void VTracking::update_meters(ptstime pts)
{
	double output_levels[MAXCHANNELS];

	if(get_playback_engine()->get_output_levels(output_levels, pts))
		vwindow->gui->meters->update(output_levels);
}

void VTracking::stop_meters()
{
	vwindow->gui->meters->stop_meters();
}

void VTracking::set_delays(float over_delay, float peak_delay)
{
	vwindow->gui->meters->set_delays(over_delay * tracking_rate,
		peak_delay * tracking_rate);
}
