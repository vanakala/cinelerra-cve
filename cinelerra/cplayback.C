#include "cplayback.h"
#include "ctracking.h"
#include "cwindow.h"
#include "cwindowgui.h"
#include "edl.h"
#include "edlsession.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "playtransport.h"
#include "trackcanvas.h"
#include "transportque.h"

// Playback engine for composite window

CPlayback::CPlayback(MWindow *mwindow, CWindow *cwindow, Canvas *output)
 : PlaybackEngine(mwindow, output)
{
	this->cwindow = cwindow;
}

int CPlayback::create_render_engine()
{
	return PlaybackEngine::create_render_engine();
}

void CPlayback::init_cursor()
{
	mwindow->gui->lock_window("CPlayback::init_cursor");
	mwindow->gui->canvas->deactivate();
	mwindow->gui->unlock_window();
	cwindow->playback_cursor->start_playback(tracking_position);
}

void CPlayback::stop_cursor()
{
	cwindow->playback_cursor->stop_playback();
}


int CPlayback::brender_available(long position)
{
	return mwindow->brender_available(position);
}
