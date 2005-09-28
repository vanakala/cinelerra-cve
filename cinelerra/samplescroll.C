#include "edl.h"
#include "edlsession.h"
#include "samplescroll.h"
#include "localsession.h"
#include "maincursor.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "patchbay.h"
#include "mainsession.h"
#include "mtimebar.h"
#include "theme.h"
#include "trackcanvas.h"
#include "tracks.h"

SampleScroll::SampleScroll(MWindow *mwindow, 
	MWindowGUI *gui, 
	int x,
	int y,
	int w)
 : BC_ScrollBar(x, 
 	y,
	SCROLL_HORIZ, 
	w, 
	0, 
	0, 
	0)
{
	this->gui = gui;
	this->mwindow = mwindow;
	oldposition = 0;
}

SampleScroll::~SampleScroll()
{
}

int SampleScroll::flip_vertical()
{
	return 0;
}

int SampleScroll::in_use()
{
	return in_use();
}


int SampleScroll::resize_event()
{
	reposition_window(mwindow->theme->mhscroll_x,
		mwindow->theme->mhscroll_y, 
		mwindow->theme->mhscroll_w);
	return 0;
}

int SampleScroll::set_position()
{
	if(!gui->canvas) return 0;
	long length = Units::round(mwindow->edl->tracks->total_length() * 
		mwindow->edl->session->sample_rate / 
		mwindow->edl->local_session->zoom_sample);
	long position = mwindow->edl->local_session->view_start;
	long handle_size = mwindow->theme->mcanvas_w - 
		BC_ScrollBar::get_span(SCROLL_VERT);

	update_length(length, 
			position, 
			handle_size);

	oldposition = position;
	return 0;
}

int SampleScroll::handle_event()
{
//printf("SampleScroll::handle_event 1 %d %d\n", mwindow->edl->session->sample_rate, get_value());
	mwindow->edl->local_session->view_start = get_value();


//printf("SampleScroll::handle_event 1 %ld\n", mwindow->edl->local_session->view_start);
	mwindow->gui->canvas->draw();
	mwindow->gui->cursor->draw();

//printf("SampleScroll::handle_event 1\n");
	mwindow->gui->canvas->flash();

//printf("SampleScroll::handle_event 1\n");
	mwindow->gui->patchbay->update();

//printf("SampleScroll::handle_event 1\n");
	mwindow->gui->timebar->update();


//printf("SampleScroll::handle_event 2\n");
	return 1;
}
