// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bcsignals.h"
#include "cinelerra.h"
#include "edl.h"
#include "localsession.h"
#include "mainclock.h"
#include "meterpanel.h"
#include "renderengine.h"
#include "vplayback.h"
#include "vtimebar.h"
#include "vtracking.h"
#include "vwindow.h"
#include "vwindowgui.h"


VTracking::VTracking(VWindow *vwindow)
 : Tracking()
{
	this->vwindow = vwindow;
}

PlaybackEngine* VTracking::get_playback_engine()
{
	return vwindow->playback_engine;
}

void VTracking::start_playback(ptstime new_position)
{
	vwindow->gui->set_resize(0);
	Tracking::start_playback(new_position);
}

void VTracking::stop_playback()
{
	Tracking::stop_playback();
	vwindow->gui->set_resize(1);
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

void VTracking::set_delays(ptstime over_delay, ptstime peak_delay)
{
	int over = round(over_delay * tracking_rate);
	int peak = round(peak_delay * tracking_rate);

	vwindow->gui->meters->set_delays(over, peak);
}
