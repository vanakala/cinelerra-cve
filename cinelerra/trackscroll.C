#include "edl.h"
#include "edlsession.h"
#include "localsession.h"
#include "maincursor.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "patchbay.h"
#include "mainsession.h"
#include "theme.h"
#include "trackcanvas.h"
#include "tracks.h"
#include "trackscroll.h"

TrackScroll::TrackScroll(MWindow *mwindow, MWindowGUI *gui, int x, int y, int h)
 : BC_ScrollBar(x, 
 	y, 
	SCROLL_VERT, 
	h, 
	0, 
	0, 
	0)
{
	this->mwindow = mwindow;
	this->gui = gui;
	old_position = 0;
}

TrackScroll::~TrackScroll()
{
}

long TrackScroll::get_distance()
{
	return get_value() - old_position;
}

int TrackScroll::update()
{
	return 0;
}

int TrackScroll::resize_event()
{
	reposition_window(mwindow->theme->mvscroll_x, 
		mwindow->theme->mvscroll_y, 
		mwindow->theme->mvscroll_h);
	update();
	return 0;
}

int TrackScroll::flip_vertical(int top, int bottom)
{
	return 0;
}

int TrackScroll::handle_event()
{
	mwindow->edl->local_session->track_start = get_value();
	mwindow->edl->tracks->update_y_pixels(mwindow->theme);
	mwindow->gui->canvas->draw();
	mwindow->gui->cursor->draw();
	mwindow->gui->patchbay->update();
	mwindow->gui->canvas->flash();
// Scrollbar must be active to trap button release events
//	mwindow->gui->canvas->activate();
	old_position = get_value();
	flush();
	return 1;
}
