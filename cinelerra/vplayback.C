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

int VPlayback::create_render_engines()
{
	command->get_edl()->session->playback_strategy = PLAYBACK_LOCALHOST;
	return PlaybackEngine::create_render_engines();
}

void VPlayback::init_cursor()
{
//printf("VPlayback::init_cursor 1\n");
	vwindow->playback_cursor->start_playback(tracking_position);
//printf("VPlayback::init_cursor 2\n");
}

void VPlayback::stop_cursor()
{
	vwindow->playback_cursor->stop_playback();
}


