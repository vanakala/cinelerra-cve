#include "cwindow.h"
#include "edl.h"
#include "edlsession.h"
#include "localsession.h"
#include "mainsession.h"
#include "mbuttons.h"
#include "mtimebar.h"
#include "mwindowgui.h"
#include "mwindow.h"
#include "preferences.h"
#include "theme.h"
#include "trackcanvas.h"
#include "tracks.h"
#include "transportque.h"
#include "zoombar.h"



MTimeBar::MTimeBar(MWindow *mwindow, 
	MWindowGUI *gui,
	int x, 
	int y,
	int w,
	int h)
 : TimeBar(mwindow, gui, x, y, w, h)
{
	this->gui = gui;
}


int64_t MTimeBar::position_to_pixel(double position)
{
	return (int64_t)(position * 
		mwindow->edl->session->sample_rate / 
		mwindow->edl->local_session->zoom_sample - 
		mwindow->edl->local_session->view_start);
}


void MTimeBar::stop_playback()
{
	gui->unlock_window();
	gui->mbuttons->transport->handle_transport(STOP, 1);
	gui->lock_window();
}


void MTimeBar::draw_time()
{
//printf("TimeBar::draw_time 1\n");
	draw_range();
//printf("TimeBar::draw_time 2\n");

// fit the time
	int64_t windowspan = 0;
	int64_t timescale1 = 0;
	int64_t timescale2 = 0;
	int64_t sample_rate = 0;
	int64_t sample = 0;
	float timescale3 = 0;
	int pixel = 0;
	char string[BCTEXTLEN];
#define TIMESPACING 100

	sample_rate = mwindow->edl->session->sample_rate;
	windowspan = mwindow->edl->local_session->zoom_sample * get_w();
	timescale2 = TIMESPACING * mwindow->edl->local_session->zoom_sample;
//printf("TimeBar::draw_time 2\n");

	if(timescale2 <= sample_rate / 4) timescale1 = sample_rate / 4;
	else
	if(timescale2 <= sample_rate / 2) timescale1 = sample_rate / 2;
	else
	if(timescale2 <= sample_rate) timescale1 = sample_rate;
	else
	if(timescale2 <= sample_rate * 10) timescale1 = sample_rate * 10;
	else
	if(timescale2 <= sample_rate * 60) timescale1 = sample_rate * 60;
	else
	if(timescale2 <= sample_rate * 600) timescale1 = sample_rate * 600;
	else
	timescale1 = sample_rate * 3600;
//printf("TimeBar::draw_time 2\n");

	for(timescale3 = timescale1; timescale3 > timescale2; timescale3 /= 2)
		;
//printf("TimeBar::draw_time 2\n");

	timescale1 = (int64_t)(timescale3 * 2);
//printf("TimeBar::draw_time 2\n");

	sample = (int64_t)(mwindow->edl->local_session->view_start * 
		mwindow->edl->local_session->zoom_sample + 
		0.5);
//printf("TimeBar::draw_time 2 %lld %lld %lld\n", mwindow->edl->local_session->zoom_sample, sample_rate, timescale1);

	sample /= timescale1;
	sample *= timescale1;
	pixel = (int64_t)(sample / 
		mwindow->edl->local_session->zoom_sample - 
		mwindow->edl->local_session->view_start);
//printf("TimeBar::draw_time 2\n");

	for( ; pixel < get_w(); pixel += timescale1 / mwindow->edl->local_session->zoom_sample, sample += timescale1)
	{
		Units::totext(string, 
			sample, 
			sample_rate, 
			mwindow->edl->session->time_format, 
			mwindow->edl->session->frame_rate,
			mwindow->edl->session->frames_per_foot);
	 	set_color(BLACK);
		set_font(MEDIUMFONT);
		draw_text(pixel + 4, get_text_ascent(MEDIUMFONT), string);
		draw_line(pixel, 3, pixel, get_h() - 4);
	}
//printf("TimeBar::draw_time 200\n");
}

void MTimeBar::draw_range()
{
	int x1 = 0, x2 = 0;
	if(mwindow->edl->tracks->total_playable_vtracks() &&
		mwindow->preferences->use_brender)
	{
		double time_per_pixel = (double)mwindow->edl->local_session->zoom_sample /
			mwindow->edl->session->sample_rate;
		x1 = (int)(mwindow->edl->session->brender_start / time_per_pixel) - 
			mwindow->edl->local_session->view_start;
		x2 = (int)(mwindow->session->brender_end / time_per_pixel) - 
			mwindow->edl->local_session->view_start;
	}

//printf("MTimeBar::draw_range 1 %f %f\n", mwindow->edl->session->brender_start, mwindow->session->brender_end);
	if(x2 > x1 && 
		x1 < get_w() && 
		x2 > 0)
	{
		draw_top_background(get_parent(), 0, 0, x1, get_h());
		draw_3segmenth(x1, 0, x2 - x1, mwindow->theme->timebar_brender_data);
		draw_top_background(get_parent(), x2, 0, get_w() - x2, get_h());
	}
	else
		draw_top_background(get_parent(), 0, 0, get_w(), get_h());
//printf("MTimeBar::draw_range %f %f\n", mwindow->session->brender_end, time_per_pixel);
}

void MTimeBar::select_label(double position)
{
	EDL *edl = mwindow->edl;

	mwindow->gui->unlock_window();
	mwindow->gui->mbuttons->transport->handle_transport(STOP, 1);
	mwindow->gui->lock_window();

	position = mwindow->edl->align_to_frame(position, 1);

	if(shift_down())
	{
		if(position > edl->local_session->selectionend / 2 + edl->local_session->selectionstart / 2)
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

// Que the CWindow
	mwindow->cwindow->update(1, 0, 0);
	mwindow->gui->cursor->hide();
	mwindow->gui->cursor->draw();
	mwindow->gui->canvas->activate();
	mwindow->gui->zoombar->update();
	update_highlights();
	mwindow->gui->canvas->flash();
}


int MTimeBar::resize_event()
{
	reposition_window(mwindow->theme->mtimebar_x,
		mwindow->theme->mtimebar_y,
		mwindow->theme->mtimebar_w,
		mwindow->theme->mtimebar_h);
	update();
	return 1;
}

int MTimeBar::test_preview(int buttonpress)
{
	int result = 0;
	return result;
}











