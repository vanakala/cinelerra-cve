#include "edl.h"
#include "edlsession.h"
#include "localsession.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "mainsession.h"
#include "mtimebar.h"
#include "preferences.h"
#include "theme.h"
#include "trackcanvas.h"
#include "tracks.h"
#include "units.h"
#include "zoombar.h"





ZoomBar::ZoomBar(MWindow *mwindow, MWindowGUI *gui)
 : BC_SubWindow(mwindow->theme->mzoom_x,
 	mwindow->theme->mzoom_y,
	mwindow->theme->mzoom_w,
	mwindow->theme->mzoom_h) 
{
	this->gui = gui;
	this->mwindow = mwindow;
	old_position = 0;
}

ZoomBar::~ZoomBar()
{
	delete sample_zoom;
	delete amp_zoom;
	delete track_zoom;
}

int ZoomBar::create_objects()
{
	int x = 3, y = 1;

	draw_top_background(get_parent(), 0, 0, get_w(), get_h());
	sample_zoom = new SampleZoomPanel(mwindow, this, x, y);
	sample_zoom->create_objects();
	x += sample_zoom->get_w();
	amp_zoom = new AmpZoomPanel(mwindow, this, x, y);
	amp_zoom->create_objects();
	x += amp_zoom->get_w();
	track_zoom = new TrackZoomPanel(mwindow, this, x, y);
	track_zoom->create_objects();
	x += track_zoom->get_w() + 5;

// FIXME
	add_subwindow(from_value = new FromTextBox(mwindow, this, x, y));
	x += from_value->get_w() + 5;
	add_subwindow(length_value = new LengthTextBox(mwindow, this, x, y));
	x += length_value->get_w() + 5;
	add_subwindow(to_value = new ToTextBox(mwindow, this, x, y));
	x += to_value->get_w() + 5;

	add_subwindow(playback_value = new BC_Title(x, 100, "--", MEDIUMFONT, RED));

	add_subwindow(zoom_value = new BC_Title(x, 100, "--", MEDIUMFONT, BLACK));
	update();
	return 0;
}


void ZoomBar::resize_event()
{
	reposition_window(mwindow->theme->mzoom_x,
		mwindow->theme->mzoom_y,
		mwindow->theme->mzoom_w,
		mwindow->theme->mzoom_h);

	draw_top_background(get_parent(), 0, 0, get_w(), get_h());
	int x = 3, y = 1;
	sample_zoom->reposition_window(x, y);
	x += sample_zoom->get_w();
	amp_zoom->reposition_window(x, y);
	x += amp_zoom->get_w();
	track_zoom->reposition_window(x, y);
	flash();
}

void ZoomBar::redraw_time_dependancies()
{
// Recalculate sample zoom menu
	sample_zoom->update_menu();
	sample_zoom->update(mwindow->edl->local_session->zoom_sample);
	update_clocks();
}

int ZoomBar::draw()
{
	update();
	return 0;
}

int ZoomBar::update()
{
//printf("ZoomBar::update 1 %f\n", mwindow->edl->local_session->selectionstart);
	sample_zoom->update(mwindow->edl->local_session->zoom_sample);
	amp_zoom->update(mwindow->edl->local_session->zoom_y);
	track_zoom->update(mwindow->edl->local_session->zoom_track);
	update_clocks();
	return 0;
}

int ZoomBar::update_clocks()
{
//printf("ZoomBar::update_clocks 1 %f\n", mwindow->edl->local_session->selectionstart);
	from_value->update_position(mwindow->edl->local_session->selectionstart);
	length_value->update_position(mwindow->edl->local_session->selectionend - 
		mwindow->edl->local_session->selectionstart);
	to_value->update_position(mwindow->edl->local_session->selectionend);
	return 0;
}

int ZoomBar::update_playback(long new_position)
{
	if(new_position != old_position)
	{
		Units::totext(string, 
				new_position, 
				mwindow->edl->session->sample_rate, 
				mwindow->edl->session->time_format, 
				mwindow->edl->session->frame_rate,
				mwindow->edl->session->frames_per_foot);
		playback_value->update(string);
		old_position = new_position;
	}
	return 0;
}

int ZoomBar::resize_event(int w, int h)
{
// don't change anything but y and width
	reposition_window(0, h - this->get_h(), w, this->get_h());
	return 0;
}


// Values for which_one
#define SET_FROM 1
#define SET_LENGTH 2
#define SET_TO 3


int ZoomBar::set_selection(int which_one)
{
	double start_position = mwindow->edl->local_session->selectionstart;
	double end_position = mwindow->edl->local_session->selectionend;
	double length = end_position - start_position;

// Fix bogus results
// printf("ZoomBar::set_selection 1 %f %f %f\n", 
// mwindow->edl->local_session->selectionstart, 
// mwindow->edl->local_session->selectionend, 
// length);

	switch(which_one)
	{
		case SET_LENGTH:
			start_position = Units::text_to_seconds(from_value->get_text(), 
				mwindow->edl->session->sample_rate, 
				mwindow->edl->session->time_format, 
				mwindow->edl->session->frame_rate,
				mwindow->edl->session->frames_per_foot);
			length = Units::text_to_seconds(length_value->get_text(), 
				mwindow->edl->session->sample_rate, 
				mwindow->edl->session->time_format, 
				mwindow->edl->session->frame_rate,
				mwindow->edl->session->frames_per_foot);
			end_position = start_position + length;

			if(end_position < start_position)
			{
				start_position = end_position;
				mwindow->edl->local_session->selectionend = mwindow->edl->local_session->selectionstart;
			}
			break;

		case SET_FROM:
			start_position = Units::text_to_seconds(from_value->get_text(), 
				mwindow->edl->session->sample_rate, 
				mwindow->edl->session->time_format, 
				mwindow->edl->session->frame_rate,
				mwindow->edl->session->frames_per_foot);
			end_position = Units::text_to_seconds(to_value->get_text(), 
				mwindow->edl->session->sample_rate, 
				mwindow->edl->session->time_format, 
				mwindow->edl->session->frame_rate,
				mwindow->edl->session->frames_per_foot);

			if(end_position < start_position)
			{
				end_position = start_position;
				mwindow->edl->local_session->selectionend = mwindow->edl->local_session->selectionstart;
			}
			break;

		case SET_TO:
			start_position = Units::text_to_seconds(from_value->get_text(), 
				mwindow->edl->session->sample_rate, 
				mwindow->edl->session->time_format, 
				mwindow->edl->session->frame_rate,
				mwindow->edl->session->frames_per_foot);
			end_position = Units::text_to_seconds(to_value->get_text(), 
				mwindow->edl->session->sample_rate, 
				mwindow->edl->session->time_format, 
				mwindow->edl->session->frame_rate,
				mwindow->edl->session->frames_per_foot);

			if(end_position < start_position)
			{
				start_position = end_position;
				mwindow->edl->local_session->selectionend = mwindow->edl->local_session->selectionstart;
			}
			break;
	}

	mwindow->edl->local_session->selectionstart = 
		mwindow->edl->align_to_frame(start_position, 1);
	mwindow->edl->local_session->selectionend = 
		mwindow->edl->align_to_frame(end_position, 1);

// printf("ZoomBar::set_selection 2 %f %f\n", 
// mwindow->edl->local_session->selectionstart, 
// mwindow->edl->local_session->selectionend);

	mwindow->gui->timebar->update_highlights();
	mwindow->gui->cursor->hide();
	mwindow->gui->cursor->show();
	update();
	mwindow->sync_parameters(CHANGE_PARAMS);
	mwindow->gui->canvas->flash();

	return 0;
}












SampleZoomPanel::SampleZoomPanel(MWindow *mwindow, 
	ZoomBar *zoombar, 
	int x, 
	int y)
 : ZoomPanel(mwindow, 
 	zoombar, 
	mwindow->edl->local_session->zoom_sample, 
	x, 
	y, 
	120, 
	MIN_ZOOM_TIME, 
	MAX_ZOOM_TIME, 
	ZOOM_TIME)
{
	this->mwindow = mwindow;
	this->zoombar = zoombar;
}
SampleZoomPanel::~SampleZoomPanel()
{
}
int SampleZoomPanel::handle_event()
{
	mwindow->zoom_sample((long)get_value());
	return 1;
}











AmpZoomPanel::AmpZoomPanel(MWindow *mwindow, ZoomBar *zoombar, int x, int y)
 : ZoomPanel(mwindow, 
 	zoombar, 
	mwindow->edl->local_session->zoom_y, 
	x, 
	y, 
	90,
	MIN_AMP_ZOOM, 
	MAX_AMP_ZOOM, 
	ZOOM_LONG)
{
	this->mwindow = mwindow;
	this->zoombar = zoombar;
}
AmpZoomPanel::~AmpZoomPanel()
{
}
int AmpZoomPanel::handle_event()
{
	mwindow->zoom_amp((long)get_value());
	return 1;
}

TrackZoomPanel::TrackZoomPanel(MWindow *mwindow, ZoomBar *zoombar, int x, int y)
 : ZoomPanel(mwindow, 
 	zoombar, 
	mwindow->edl->local_session->zoom_track, 
	x, 
	y, 
	80,
	MIN_TRACK_ZOOM, 
	MAX_TRACK_ZOOM, 
	ZOOM_LONG)
{
	this->mwindow = mwindow;
	this->zoombar = zoombar;
}
TrackZoomPanel::~TrackZoomPanel()
{
}
int TrackZoomPanel::handle_event()
{
	mwindow->zoom_track((long)get_value());
	zoombar->amp_zoom->update(mwindow->edl->local_session->zoom_y);
	return 1;
}






FromTextBox::FromTextBox(MWindow *mwindow, ZoomBar *zoombar, int x, int y)
 : BC_TextBox(x, y, 90, 1, "")
{
	this->mwindow = mwindow;
	this->zoombar = zoombar;
}

int FromTextBox::handle_event()
{
	if(get_keypress() == 13)
	{
		zoombar->set_selection(SET_FROM);
		return 1;
	}
	return 0;
}

int FromTextBox::update_position(double new_position)
{
	Units::totext(string, 
		new_position, 
		mwindow->edl->session->time_format, 
		mwindow->edl->session->sample_rate, 
		mwindow->edl->session->frame_rate,
		mwindow->edl->session->frames_per_foot);
//printf("FromTextBox::update_position %f %s\n", new_position, string);
	update(string);
	return 0;
}






LengthTextBox::LengthTextBox(MWindow *mwindow, ZoomBar *zoombar, int x, int y)
 : BC_TextBox(x, y, 90, 1, "")
{
	this->mwindow = mwindow;
	this->zoombar = zoombar;
}

int LengthTextBox::handle_event()
{
	if(get_keypress() == 13)
	{
		zoombar->set_selection(SET_LENGTH);
		return 1;
	}
	return 0;
}

int LengthTextBox::update_position(double new_position)
{
	Units::totext(string, 
		new_position, 
		mwindow->edl->session->time_format, 
		mwindow->edl->session->sample_rate, 
		mwindow->edl->session->frame_rate,
		mwindow->edl->session->frames_per_foot);
	update(string);
	return 0;
}





ToTextBox::ToTextBox(MWindow *mwindow, ZoomBar *zoombar, int x, int y)
 : BC_TextBox(x, y, 90, 1, "")
{
	this->mwindow = mwindow;
	this->zoombar = zoombar;
}

int ToTextBox::handle_event()
{
	if(get_keypress() == 13)
	{
		zoombar->set_selection(SET_TO);
		return 1;
	}
	return 0;
}

int ToTextBox::update_position(double new_position)
{
	Units::totext(string, 
		new_position, 
		mwindow->edl->session->time_format, 
		mwindow->edl->session->sample_rate, 
		mwindow->edl->session->frame_rate,
		mwindow->edl->session->frames_per_foot);
	update(string);
	return 0;
}
