#include "edl.h"
#include "edlsession.h"
#include "localsession.h"
#include "mainclock.h"
#include "meterpanel.h"
#include "mwindow.h"
#include "vplayback.h"
#include "vtracking.h"
#include "vwindow.h"
#include "vwindowgui.h"


VTracking::VTracking(MWindow *mwindow, VWindow *vwindow)
 : Tracking(mwindow)
{
	this->vwindow = vwindow;
}

VTracking::~VTracking()
{
}

PlaybackEngine* VTracking::get_playback_engine()
{
	return vwindow->playback_engine;
}

void VTracking::update_tracker(double position)
{
//printf("VTracking::update_tracker %ld\n", position);
	vwindow->gui->lock_window();
	vwindow->get_edl()->local_session->selectionstart = 
		vwindow->get_edl()->local_session->selectionend = 
		position;
	vwindow->gui->slider->update(position);
	vwindow->gui->clock->update(position);
	vwindow->gui->unlock_window();

	update_meters((long)(position * mwindow->edl->session->frame_rate));
}

void VTracking::update_meters(long position)
{
	double output_levels[MAXCHANNELS];
	int do_audio = get_playback_engine()->get_output_levels(output_levels, position);
	if(do_audio)
	{
		vwindow->gui->lock_window();
		vwindow->gui->meters->update(output_levels);
		vwindow->gui->unlock_window();
	}
}

void VTracking::stop_meters()
{
	vwindow->gui->lock_window();
	vwindow->gui->meters->stop_meters();
	vwindow->gui->unlock_window();
}

void VTracking::draw()
{
}
