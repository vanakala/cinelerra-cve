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
