#include "ctimebar.h"
#include "cwindow.h"
#include "cwindowgui.h"
#include "edl.h"
#include "localsession.h"
#include "mbuttons.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "theme.h"


CTimeBar::CTimeBar(MWindow *mwindow, 
		CWindowGUI *gui,
		int x,
		int y,
		int w, 
		int h)
 : TimeBar(mwindow, 
		gui,
		x, 
		y,
		w,
		h)
{
	this->mwindow = mwindow;
	this->gui = gui;
}

int CTimeBar::resize_event()
{
	reposition_window(mwindow->theme->ctimebar_x,
		mwindow->theme->ctimebar_y,
		mwindow->theme->ctimebar_w,
		mwindow->theme->ctimebar_h);
	update();
	return 1;
}


EDL* CTimeBar::get_edl()
{
	return mwindow->edl;
}

void CTimeBar::draw_time()
{
	get_edl_length();
	draw_range();
}


void CTimeBar::update_preview()
{
	gui->slider->set_position();
}


void CTimeBar::select_label(double position)
{
	EDL *edl = mwindow->edl;

//printf("CTimeBar::select_label 1\n");
	gui->unlock_window();
	mwindow->gui->mbuttons->transport->handle_transport(STOP, 1);
	gui->lock_window();

//printf("CTimeBar::select_label 1\n");
	position = mwindow->edl->align_to_frame(position, 1);

//printf("CTimeBar::select_label 1\n");
	if(shift_down())
	{
		if(position > edl->local_session->selectionend / 2 + 
			edl->local_session->selectionstart / 2)
		{
		
			edl->local_session->selectionend = position;
		}
		else
		{
			edl->local_session->selectionstart = position;
		}
	}
	else
	{
		edl->local_session->selectionstart = edl->local_session->selectionend = position;
	}

//printf("CTimeBar::select_label 1\n");
// Que the CWindow
	mwindow->cwindow->update(1, 0, 0, 0, 1);

//printf("CTimeBar::select_label 1\n");

	mwindow->gui->lock_window();
	mwindow->gui->cursor->hide();
	mwindow->gui->cursor->draw();
	mwindow->gui->update(0,
		1,      // 1 for incremental drawing.  2 for full refresh
		1,
		0,
		1, 
		1,
		0);
	mwindow->gui->unlock_window();
//printf("CTimeBar::select_label 2\n");
}





