
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */

#include "automation.h"
#include "bcmenuitem.h"
#include "bcsignals.h"
#include "clip.h"
#include "edl.h"
#include "edlsession.h"
#include "keys.h"
#include "language.h"
#include "localsession.h"
#include "maincursor.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "mainsession.h"
#include "mtimebar.h"
#include "preferences.h"
#include "patchbay.h"
#include "theme.h"
#include "trackcanvas.h"
#include "units.h"
#include "vframe.h"
#include "zoombar.h"


ZoomBar::ZoomBar(MWindow *mwindow, MWindowGUI *gui)
 : BC_SubWindow(mwindow->theme->mzoom_x,
	mwindow->theme->mzoom_y,
	mwindow->theme->mzoom_w,
	mwindow->theme->mzoom_h) 
{
	this->gui = gui;
	this->mwindow = mwindow;
	sample_zoom = 0;
	amp_zoom = 0;
	track_zoom = 0;
}

ZoomBar::~ZoomBar()
{
	delete sample_zoom;
	delete amp_zoom;
	delete track_zoom;
}

void ZoomBar::show()
{
	int x = 3;
	int y = get_h() / 2 - 
		mwindow->theme->get_image_set("zoombar_menu", 0)[0]->get_h() / 2;

	draw_top_background(get_parent(), 0, 0, get_w(), get_h());
	sample_zoom = new SampleZoomPanel(mwindow, this, x, y);
	sample_zoom->zoom_text->set_tooltip(_("Duration visible in the timeline"));
	sample_zoom->zoom_tumbler->set_tooltip(_("Duration visible in the timeline"));
	x += sample_zoom->get_w();
	amp_zoom = new AmpZoomPanel(mwindow, this, x, y);
	amp_zoom->zoom_text->set_tooltip(_("Audio waveform scale"));
	amp_zoom->zoom_tumbler->set_tooltip(_("Audio waveform scale"));
	x += amp_zoom->get_w();
	track_zoom = new TrackZoomPanel(mwindow, this, x, y);
	track_zoom->zoom_text->set_tooltip(_("Height of tracks in the timeline"));
	track_zoom->zoom_tumbler->set_tooltip(_("Height of tracks in the timeline"));
	x += track_zoom->get_w() + 10;
	add_subwindow(auto_type = new AutoTypeMenu(mwindow, this, x, y));
	x += auto_type->get_w() + 10;
#define DEFAULT_TEXT "000.00 to 000.00"
	add_subwindow(auto_zoom = new AutoZoom(mwindow, this, x, y, 0));
	x += auto_zoom->get_w();
	add_subwindow(auto_zoom_text = new ZoomTextBox(
		mwindow, 
		this, 
		x, 
		y,
		DEFAULT_TEXT));
	x += auto_zoom_text->get_w() + 5;
	add_subwindow(auto_zoom = new AutoZoom(mwindow, this, x, y, 1));
	update_autozoom();
	x += auto_zoom->get_w() + 5;

	add_subwindow(from_value = new FromTextBox(mwindow, this, x, y));
	x += from_value->get_w() + 5;
	add_subwindow(length_value = new LengthTextBox(mwindow, this, x, y));
	x += length_value->get_w() + 5;
	add_subwindow(to_value = new ToTextBox(mwindow, this, x, y));
	x += to_value->get_w() + 5;

	update_formatting(from_value);
	update_formatting(length_value);
	update_formatting(to_value);
	update();
}

void ZoomBar::update_formatting(BC_TextBox *dst)
{
	dst->set_separators(
		Units::format_to_separators(edlsession->time_format));
}

void ZoomBar::resize_event()
{
	reposition_window(mwindow->theme->mzoom_x,
		mwindow->theme->mzoom_y,
		mwindow->theme->mzoom_w,
		mwindow->theme->mzoom_h);

	draw_top_background(get_parent(), 0, 0, get_w(), get_h());
	int x = 3, y = 1;
	if (sample_zoom) delete sample_zoom;
	sample_zoom = new SampleZoomPanel(mwindow, this, x, y);
	flash();
}

void ZoomBar::redraw_time_dependancies()
{
// Recalculate sample zoom menu
	sample_zoom->update_menu();
	sample_zoom->update(master_edl->local_session->zoom_time);
	update_formatting(from_value);
	update_formatting(length_value);
	update_formatting(to_value);
	update_autozoom();
	update_clocks();
}

void ZoomBar::draw()
{
	update();
}

void ZoomBar::update_autozoom()
{
	char string[BCTEXTLEN];
	switch(master_edl->local_session->zoombar_showautotype)
	{
	case AUTOGROUPTYPE_AUDIO_FADE:
	case AUTOGROUPTYPE_VIDEO_FADE:
		sprintf(string, "%0.01f to %0.01f\n", 
			master_edl->local_session->automation_mins[master_edl->local_session->zoombar_showautotype],
			master_edl->local_session->automation_maxs[master_edl->local_session->zoombar_showautotype]);
		break;
	case AUTOGROUPTYPE_ZOOM:
		sprintf(string, "%0.03f to %0.03f\n", 
			master_edl->local_session->automation_mins[master_edl->local_session->zoombar_showautotype],
			master_edl->local_session->automation_maxs[master_edl->local_session->zoombar_showautotype]);
		break;
	case AUTOGROUPTYPE_X:
	case AUTOGROUPTYPE_Y:
		sprintf(string, "%0.0f to %.0f\n", 
			master_edl->local_session->automation_mins[master_edl->local_session->zoombar_showautotype],
			master_edl->local_session->automation_maxs[master_edl->local_session->zoombar_showautotype]);
		break;
	}
	auto_zoom_text->update(string);
}

void ZoomBar::update()
{
	sample_zoom->update(master_edl->local_session->zoom_time);
	amp_zoom->update(master_edl->local_session->zoom_y);
	track_zoom->update(master_edl->local_session->zoom_track);
	update_autozoom();
	update_clocks();
}

void ZoomBar::update_clocks()
{
	from_value->update_position(master_edl->local_session->get_selectionstart(1));
	length_value->update_position(master_edl->local_session->get_selectionend(1) -
		master_edl->local_session->get_selectionstart(1));
	to_value->update_position(master_edl->local_session->get_selectionend(1));
}

void ZoomBar::resize_event(int w, int h)
{
// don't change anything but y and width
	reposition_window(0, h - this->get_h(), w, this->get_h());
}

// Values for which_one
#define SET_FROM 1
#define SET_LENGTH 2
#define SET_TO 3

void ZoomBar::set_selection(int which_one)
{
	ptstime start_position = master_edl->local_session->get_selectionstart(1);
	ptstime end_position = master_edl->local_session->get_selectionend(1);
	ptstime length = end_position - start_position;

// Fix bogus results

	switch(which_one)
	{
	case SET_LENGTH:
		start_position = Units::text_to_seconds(from_value->get_text(), 
			edlsession->sample_rate,
			edlsession->time_format,
			edlsession->frame_rate,
			edlsession->frames_per_foot);
		length = Units::text_to_seconds(length_value->get_text(),
			edlsession->sample_rate,
			edlsession->time_format,
			edlsession->frame_rate,
			edlsession->frames_per_foot);
		end_position = start_position + length;

		if(end_position < start_position)
		{
			start_position = end_position;
			master_edl->local_session->set_selectionend(
				master_edl->local_session->get_selectionstart(1));
		}
		break;

	case SET_FROM:
		start_position = Units::text_to_seconds(from_value->get_text(), 
			edlsession->sample_rate,
			edlsession->time_format,
			edlsession->frame_rate,
			edlsession->frames_per_foot);
		end_position = Units::text_to_seconds(to_value->get_text(), 
			edlsession->sample_rate,
			edlsession->time_format,
			edlsession->frame_rate,
			edlsession->frames_per_foot);

		if(end_position < start_position)
		{
			end_position = start_position;
			master_edl->local_session->set_selectionend(
				master_edl->local_session->get_selectionstart(1));
		}
		break;

	case SET_TO:
		start_position = Units::text_to_seconds(from_value->get_text(),
			edlsession->sample_rate,
			edlsession->time_format,
			edlsession->frame_rate,
			edlsession->frames_per_foot);
		end_position = Units::text_to_seconds(to_value->get_text(),
			edlsession->sample_rate,
			edlsession->time_format,
			edlsession->frame_rate,
			edlsession->frames_per_foot);

		if(end_position < start_position)
		{
			start_position = end_position;
			master_edl->local_session->set_selectionend(
				master_edl->local_session->get_selectionstart(1));
		}
		break;
	}

	master_edl->local_session->set_selectionstart(
		master_edl->align_to_frame(start_position));
	master_edl->local_session->set_selectionend(
		master_edl->align_to_frame(end_position));

	mwindow->gui->timebar->update_highlights();
	mwindow->gui->cursor->update();
	update();
	mwindow->sync_parameters(0);
	mwindow->gui->canvas->flash();
}


SampleZoomPanel::SampleZoomPanel(MWindow *mwindow, 
	ZoomBar *zoombar, 
	int x, 
	int y)
 : ZoomPanel(mwindow, 
	zoombar,
	master_edl->local_session->zoom_time,
	x, 
	y, 
	110, 
	MIN_ZOOM_TIME, 
	MAX_ZOOM_TIME, 
	ZOOM_TIME,
	0,
	mwindow->theme->get_image_set("zoombar_menu", 0),
	mwindow->theme->get_image_set("zoombar_tumbler", 0))
{
	this->mwindow = mwindow;
	this->zoombar = zoombar;
}

int SampleZoomPanel::handle_event()
{
	mwindow->zoom_time((ptstime)get_value());
	return 1;
}


AmpZoomPanel::AmpZoomPanel(MWindow *mwindow, ZoomBar *zoombar, int x, int y)
 : ZoomPanel(mwindow, 
	zoombar,
	master_edl->local_session->zoom_y,
	x, 
	y, 
	80,
	MIN_AMP_ZOOM, 
	MAX_AMP_ZOOM, 
	ZOOM_LONG,
	0,
	mwindow->theme->get_image_set("zoombar_menu", 0),
	mwindow->theme->get_image_set("zoombar_tumbler", 0))
{
	this->mwindow = mwindow;
	this->zoombar = zoombar;
}

int AmpZoomPanel::handle_event()
{
	mwindow->zoom_amp(get_value());
	return 1;
}


TrackZoomPanel::TrackZoomPanel(MWindow *mwindow, ZoomBar *zoombar, int x, int y)
 : ZoomPanel(mwindow, 
	zoombar, 
	master_edl->local_session->zoom_track,
	x, 
	y, 
	70,
	MIN_TRACK_ZOOM, 
	MAX_TRACK_ZOOM, 
	ZOOM_LONG,
	0,
	mwindow->theme->get_image_set("zoombar_menu", 0),
	mwindow->theme->get_image_set("zoombar_tumbler", 0))
{
	this->mwindow = mwindow;
	this->zoombar = zoombar;
}

int TrackZoomPanel::handle_event()
{
	mwindow->zoom_track(get_value());
	zoombar->amp_zoom->update(master_edl->local_session->zoom_y);
	return 1;
}


AutoZoom::AutoZoom(MWindow *mwindow, ZoomBar *zoombar, int x, int y, int changemax)
 : BC_Tumbler(x,
	y,
	mwindow->theme->get_image_set("zoombar_tumbler"))
{
	this->mwindow = mwindow;
	this->zoombar = zoombar;
	this->changemax = changemax;
	if (changemax)
		set_tooltip(_("Automation range maximum"));
	else
		set_tooltip(_("Automation range minimum"));
}

void AutoZoom::handle_up_event()
{
	mwindow->change_currentautorange(master_edl->local_session->zoombar_showautotype, 1, changemax);

	mwindow->gui->zoombar->update_autozoom();
	mwindow->draw_canvas_overlays();
	mwindow->gui->patchbay->update();
}

void AutoZoom::handle_down_event()
{
	mwindow->change_currentautorange(master_edl->local_session->zoombar_showautotype, 0, changemax);

	mwindow->gui->zoombar->update_autozoom();
	mwindow->gui->patchbay->update();
	mwindow->draw_canvas_overlays();
}


AutoTypeMenu::AutoTypeMenu(MWindow *mwindow, ZoomBar *zoombar, int x, int y)
	: BC_PopupMenu(x, y, 120,to_text(master_edl->local_session->zoombar_showautotype), 1)
{
	this->mwindow = mwindow;
	this->zoombar = zoombar;
	set_tooltip(_("Automation Type"));
	add_item(new BC_MenuItem(to_text(AUTOGROUPTYPE_AUDIO_FADE)));
	add_item(new BC_MenuItem(to_text(AUTOGROUPTYPE_VIDEO_FADE)));
	add_item(new BC_MenuItem(to_text(AUTOGROUPTYPE_ZOOM)));
	add_item(new BC_MenuItem(to_text(AUTOGROUPTYPE_X)));
	add_item(new BC_MenuItem(to_text(AUTOGROUPTYPE_Y)));
}

const char* AutoTypeMenu::to_text(int mode)
{
	switch(mode)
	{
	case AUTOGROUPTYPE_AUDIO_FADE:
		return _("Audio Fade:");
	case AUTOGROUPTYPE_VIDEO_FADE:
		return _("Video Fade:");
	case AUTOGROUPTYPE_ZOOM:
		return _("Zoom:");
	case AUTOGROUPTYPE_X:
		return _("X:");
	case AUTOGROUPTYPE_Y:
		return _("Y:");
	default:
		return _("??");
	}
}

int AutoTypeMenu::from_text(const char *text)
{
	if(!strcmp(text, to_text(AUTOGROUPTYPE_AUDIO_FADE)))
		return AUTOGROUPTYPE_AUDIO_FADE;
	if(!strcmp(text, to_text(AUTOGROUPTYPE_VIDEO_FADE)))
		return AUTOGROUPTYPE_VIDEO_FADE;
	if(!strcmp(text, to_text(AUTOGROUPTYPE_ZOOM)))
		return AUTOGROUPTYPE_ZOOM;
	if(!strcmp(text, to_text(AUTOGROUPTYPE_X)))
		return AUTOGROUPTYPE_X;
	if(!strcmp(text, to_text(AUTOGROUPTYPE_Y)))
		return AUTOGROUPTYPE_Y;
	return AUTOGROUPTYPE_COUNT;
}

int AutoTypeMenu::handle_event()
{
	master_edl->local_session->zoombar_showautotype = from_text(this->get_text());
	this->zoombar->update_autozoom();
	return 1;
}


ZoomTextBox::ZoomTextBox(MWindow *mwindow, ZoomBar *zoombar, int x, int y, const char *text)
 : BC_TextBox(x, y, 130, 1, text)
{
	this->mwindow = mwindow;
	this->zoombar = zoombar;
	set_tooltip(_("Automation range"));
}

int ZoomTextBox::button_press_event()
{
	int cursor_x, cursor_y;

	if (!(get_buttonpress() == 4 || get_buttonpress() == 5)) {
		BC_TextBox::button_press_event();
		return 0;
	}
	if (!is_event_win()) return 0;

	int changemax = 1;
	get_relative_cursor_pos(&cursor_x, &cursor_y);
	if(cursor_x < get_w()/2)
		changemax = 0;

	// increment
	if (get_buttonpress() == 4)
		mwindow->change_currentautorange(master_edl->local_session->zoombar_showautotype, 1, changemax);

	// decrement
	if (get_buttonpress() == 5)
		mwindow->change_currentautorange(master_edl->local_session->zoombar_showautotype, 0, changemax);

	mwindow->gui->zoombar->update_autozoom();
	mwindow->gui->patchbay->update();
	mwindow->draw_canvas_overlays();
	return 1;
}

int ZoomTextBox::handle_event()
{
	float min, max;

	if (sscanf(this->get_text(),"%f to%f",&min, &max) == 2)
	{
		AUTOMATIONVIEWCLAMPS(min, master_edl->local_session->zoombar_showautotype);
		AUTOMATIONVIEWCLAMPS(max, master_edl->local_session->zoombar_showautotype);
		if (max > min) 
		{
			master_edl->local_session->automation_mins[master_edl->local_session->zoombar_showautotype] = min;
			master_edl->local_session->automation_maxs[master_edl->local_session->zoombar_showautotype] = max;
			mwindow->gui->zoombar->update_autozoom();
			mwindow->gui->patchbay->update();
			mwindow->draw_canvas_overlays();
		}
	}
	// TODO: Make the text turn red when it's a bad range..
	return 1;
}


FromTextBox::FromTextBox(MWindow *mwindow, ZoomBar *zoombar, int x, int y)
 : BC_TextBox(x, y, 90, 1, "")
{
	this->mwindow = mwindow;
	this->zoombar = zoombar;
	set_tooltip(_("Selection start time"));
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

void FromTextBox::update_position(ptstime new_position)
{
	char string[256];

	new_position += edlsession->get_frame_offset();
	edlsession->ptstotext(string, new_position);
	update(string);
}


LengthTextBox::LengthTextBox(MWindow *mwindow, ZoomBar *zoombar, int x, int y)
 : BC_TextBox(x, y, 90, 1, "")
{
	this->mwindow = mwindow;
	this->zoombar = zoombar;
	set_tooltip(_("Selection length"));
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

void LengthTextBox::update_position(ptstime new_position)
{
	char string[256];

	edlsession->ptstotext(string, new_position);
	update(string);
}


ToTextBox::ToTextBox(MWindow *mwindow, ZoomBar *zoombar, int x, int y)
 : BC_TextBox(x, y, 90, 1, "")
{
	this->mwindow = mwindow;
	this->zoombar = zoombar;
	set_tooltip(_("Selection end time"));
}

int ToTextBox::handle_event()
{
	if(get_keypress() == RETURN)
	{
		zoombar->set_selection(SET_TO);
		return 1;
	}
	return 0;
}

void ToTextBox::update_position(ptstime new_position)
{
	char string[256];

	new_position += edlsession->get_frame_offset() /
		edlsession->frame_rate;
	edlsession->ptstotext(string, new_position);
	update(string);
}
