#include "edl.h"
#include "edlsession.h"
#include "localsession.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "patchbay.h"
#include "mainsession.h"
#include "theme.h"
#include "trackcanvas.h"
#include "tracks.h"
#include "trackscroll.h"

TrackScroll::TrackScroll(MWindow *mwindow, MWindowGUI *gui, int pixels)
 : BC_ScrollBar(mwindow->theme->mcanvas_x + 
 		mwindow->theme->mcanvas_w - 
		BC_ScrollBar::get_span(SCROLL_VERT), 
 	mwindow->theme->mcanvas_y, 
	SCROLL_VERT, 
	pixels, 
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
	reposition_window(mwindow->theme->mcanvas_x + 
			mwindow->theme->mcanvas_w - 
			BC_ScrollBar::get_span(SCROLL_VERT), 
		mwindow->theme->mcanvas_y, 
		mwindow->theme->mcanvas_h - 
			BC_ScrollBar::get_span(SCROLL_HORIZ)); // fix scrollbar
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
