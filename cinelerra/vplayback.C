#include "edl.h"
#include "edlsession.h"
#include "playtransport.h"
#include "transportque.h"
#include "vplayback.h"
#include "vtracking.h"
#include "vwindow.h"
#include "vwindowgui.h"

// Playback engine for viewer

VPlayback::VPlayback(MWindow *mwindow, VWindow *vwindow, Canvas *output)
 : PlaybackEngine(mwindow, output)
{
	this->vwindow = vwindow;
}

int VPlayback::create_render_engine()
{
	return PlaybackEngine::create_render_engine();
}

void VPlayback::init_cursor()
{
	vwindow->playback_cursor->start_playback(tracking_position);
}

void VPlayback::stop_cursor()
{
	vwindow->playback_cursor->stop_playback();
}


