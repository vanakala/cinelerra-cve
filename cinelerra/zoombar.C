// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

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

// Values for which_one
#define SET_FROM 1
#define SET_LENGTH 2
#define SET_TO 3

ZoomBar::ZoomBar()
 : BC_SubWindow(theme_global->mzoom_x, theme_global->mzoom_y,
	theme_global->mzoom_w, theme_global->mzoom_h)
{
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
		theme_global->get_image_set("zoombar_menu", 0)[0]->get_h() / 2;

	draw_top_background(get_parent(), 0, 0, get_w(), get_h());
	sample_zoom = new SampleZoomPanel(this, x, y);
	sample_zoom->zoom_text->set_tooltip(_("Duration visible in the timeline"));
	sample_zoom->zoom_tumbler->set_tooltip(_("Duration visible in the timeline"));
	x += sample_zoom->get_w();
	amp_zoom = new AmpZoomPanel(this, x, y);
	amp_zoom->zoom_text->set_tooltip(_("Audio waveform scale"));
	amp_zoom->zoom_tumbler->set_tooltip(_("Audio waveform scale"));
	x += amp_zoom->get_w();
	track_zoom = new TrackZoomPanel(this, x, y);
	track_zoom->zoom_text->set_tooltip(_("Height of tracks in the timeline"));
	track_zoom->zoom_tumbler->set_tooltip(_("Height of tracks in the timeline"));
	x += track_zoom->get_w() + 10;
	add_subwindow(auto_type = new AutoTypeMenu(this, x, y));
	x += auto_type->get_w() + 10;
	add_subwindow(auto_zoom = new AutoZoom(this, x, y, 0));
	x += auto_zoom->get_w();
	add_subwindow(auto_zoom_text = new ZoomTextBox(this, x, y,
		"000.00 to 000.00"));
	x += auto_zoom_text->get_w() + 5;
	add_subwindow(auto_zoom = new AutoZoom(this, x, y, 1));
	update_autozoom();
	x += auto_zoom->get_w() + 5;

	add_subwindow(from_value = new PositionTextBox(this, x, y, SET_FROM));
	x += from_value->get_w() + 5;
	add_subwindow(length_value = new PositionTextBox(this, x, y, SET_LENGTH));
	x += length_value->get_w() + 5;
	add_subwindow(to_value = new PositionTextBox(this, x, y, SET_TO));
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
	reposition_window(theme_global->mzoom_x, theme_global->mzoom_y,
		theme_global->mzoom_w, theme_global->mzoom_h);

	draw_top_background(get_parent(), 0, 0, get_w(), get_h());
	if(sample_zoom)
		delete sample_zoom;
	sample_zoom = new SampleZoomPanel(this, 3, 1);
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
			edlsession->sample_rate, edlsession->time_format,
			edlsession->frame_rate);
		length = Units::text_to_seconds(length_value->get_text(),
			edlsession->sample_rate, edlsession->time_format,
			edlsession->frame_rate);
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
			edlsession->sample_rate, edlsession->time_format,
			edlsession->frame_rate);
		end_position = Units::text_to_seconds(to_value->get_text(),
			edlsession->sample_rate, edlsession->time_format,
			edlsession->frame_rate);

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
			edlsession->frame_rate);
		end_position = Units::text_to_seconds(to_value->get_text(),
			edlsession->sample_rate,
			edlsession->time_format,
			edlsession->frame_rate);

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

	mwindow_global->update_gui(WUPD_TOGLIGHTS | WUPD_CURSOR);
	update();
	mwindow_global->sync_parameters(0);
	mwindow_global->gui->canvas->flash();
}


SampleZoomPanel::SampleZoomPanel(ZoomBar *zoombar, int x, int y)
 : ZoomPanel(zoombar, master_edl->local_session->zoom_time,
	x, y, 110, MIN_ZOOM_TIME, MAX_ZOOM_TIME, ZOOM_TIME, 0,
	theme_global->get_image_set("zoombar_menu", 0),
	theme_global->get_image_set("zoombar_tumbler", 0))
{
	this->zoombar = zoombar;
}

int SampleZoomPanel::handle_event()
{
	mwindow_global->zoom_time((ptstime)get_value());
	return 1;
}


AmpZoomPanel::AmpZoomPanel(ZoomBar *zoombar, int x, int y)
 : ZoomPanel(zoombar, master_edl->local_session->zoom_y,
	x, y, 80, MIN_AMP_ZOOM, MAX_AMP_ZOOM, ZOOM_LONG, 0,
	theme_global->get_image_set("zoombar_menu", 0),
	theme_global->get_image_set("zoombar_tumbler", 0))
{
	this->zoombar = zoombar;
}

int AmpZoomPanel::handle_event()
{
	mwindow_global->zoom_amp(get_value());
	return 1;
}


TrackZoomPanel::TrackZoomPanel(ZoomBar *zoombar, int x, int y)
 : ZoomPanel(zoombar, master_edl->local_session->zoom_track,
	x, y, 70, MIN_TRACK_ZOOM, MAX_TRACK_ZOOM, ZOOM_LONG, 0,
	theme_global->get_image_set("zoombar_menu", 0),
	theme_global->get_image_set("zoombar_tumbler", 0))
{
	this->zoombar = zoombar;
}

int TrackZoomPanel::handle_event()
{
	mwindow_global->zoom_track(get_value());
	zoombar->amp_zoom->update(master_edl->local_session->zoom_y);
	return 1;
}


AutoZoom::AutoZoom(ZoomBar *zoombar, int x, int y, int changemax)
 : BC_Tumbler(x, y, theme_global->get_image_set("zoombar_tumbler"))
{
	this->zoombar = zoombar;
	this->changemax = changemax;
	if(changemax)
		set_tooltip(_("Automation range maximum"));
	else
		set_tooltip(_("Automation range minimum"));
}

void AutoZoom::handle_up_event()
{
	mwindow_global->change_currentautorange(master_edl->local_session->zoombar_showautotype, 1, changemax);

	mwindow_global->gui->zoombar->update_autozoom();
	mwindow_global->draw_canvas_overlays();
	mwindow_global->gui->patchbay->update();
}

void AutoZoom::handle_down_event()
{
	mwindow_global->change_currentautorange(master_edl->local_session->zoombar_showautotype, 0, changemax);

	mwindow_global->gui->zoombar->update_autozoom();
	mwindow_global->gui->patchbay->update();
	mwindow_global->draw_canvas_overlays();
}


AutoTypeMenu::AutoTypeMenu(ZoomBar *zoombar, int x, int y)
 : BC_PopupMenu(x, y, 120,to_text(master_edl->local_session->zoombar_showautotype), 1)
{
	this->zoombar = zoombar;
	set_tooltip(_("Automation Type"));
	add_item(new BC_MenuItem(to_text(AUTOGROUPTYPE_AUDIO_FADE)));
	add_item(new BC_MenuItem(to_text(AUTOGROUPTYPE_VIDEO_FADE)));
	add_item(new BC_MenuItem(to_text(AUTOGROUPTYPE_ZOOM)));
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
	return AUTOGROUPTYPE_COUNT;
}

int AutoTypeMenu::handle_event()
{
	master_edl->local_session->zoombar_showautotype = from_text(this->get_text());
	zoombar->update_autozoom();
	return 1;
}


ZoomTextBox::ZoomTextBox(ZoomBar *zoombar, int x, int y, const char *text)
 : BC_TextBox(x, y, 130, 1, text)
{
	this->zoombar = zoombar;
	set_tooltip(_("Automation range"));
}

int ZoomTextBox::button_press_event()
{
	int cursor_x, cursor_y;

	if(!(get_buttonpress() == 4 || get_buttonpress() == 5))
		return BC_TextBox::button_press_event();

	if(!is_event_win())
		return 0;

	int changemax = 1;
	get_relative_cursor_pos(&cursor_x, &cursor_y);

	if(cursor_x < get_w() / 2)
		changemax = 0;

	// increment
	if (get_buttonpress() == 4)
		mwindow_global->change_currentautorange(
			master_edl->local_session->zoombar_showautotype, 1, changemax);

	// decrement
	if (get_buttonpress() == 5)
		mwindow_global->change_currentautorange(
			master_edl->local_session->zoombar_showautotype, 0, changemax);

	mwindow_global->gui->zoombar->update_autozoom();
	mwindow_global->gui->patchbay->update();
	mwindow_global->draw_canvas_overlays();
	return 1;
}

int ZoomTextBox::handle_event()
{
	double min, max;

	if(sscanf(this->get_text(),"%lf to%lf", &min, &max) == 2)
	{
		AUTOMATIONVIEWCLAMPS(min, master_edl->local_session->zoombar_showautotype);
		AUTOMATIONVIEWCLAMPS(max, master_edl->local_session->zoombar_showautotype);

		if(max > min)
		{
			master_edl->local_session->automation_mins[master_edl->local_session->zoombar_showautotype] = min;
			master_edl->local_session->automation_maxs[master_edl->local_session->zoombar_showautotype] = max;
			mwindow_global->gui->zoombar->update_autozoom();
			mwindow_global->gui->patchbay->update();
			mwindow_global->draw_canvas_overlays();
		}
	}
	// TODO: Make the text turn red when it's a bad range..
	return 1;
}


PositionTextBox::PositionTextBox(ZoomBar *zoombar, int x, int y, int set_id)
 : BC_TextBox(x, y, 90, 1, "")
{
	const char *tooltip = 0;

	switch(set_id)
	{
	case SET_FROM:
		tooltip = N_("Selection start time");
		break;
	case SET_LENGTH:
		tooltip = N_("Selection length");
		break;
	case SET_TO:
		tooltip = _("Selection end time");
		break;
	}
	this->zoombar = zoombar;
	this->set_id = set_id;
	if(tooltip)
		set_tooltip(_(tooltip));
}

int PositionTextBox::handle_event()
{
	if(get_keypress() == RETURN)
	{
		zoombar->set_selection(set_id);
		return 1;
	}
	return 0;
}

void PositionTextBox::update_position(ptstime new_position)
{
	char string[256];

	edlsession->ptstotext(string, new_position);
	update(string);
}
