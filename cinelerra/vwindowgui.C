// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "asset.h"
#include "awindowgui.h"
#include "awindow.h"
#include "bcsignals.h"
#include "canvas.h"
#include "clip.h"
#include "clipedit.h"
#include "edl.h"
#include "edlsession.h"
#include "filesystem.h"
#include "filexml.h"
#include "fonts.h"
#include "keys.h"
#include "labels.h"
#include "language.h"
#include "localsession.h"
#include "mainclock.h"
#include "mainmenu.h"
#include "mainsession.h"
#include "mainundo.h"
#include "meterpanel.h"
#include "mwindow.h"
#include "playtransport.h"
#include "preferences.h"
#include "theme.h"
#include "timebar.h"
#include "vframe.h"
#include "vplayback.h"
#include "vtimebar.h"
#include "vwindowgui.h"
#include "vwindow.h"


VWindowGUI::VWindowGUI(VWindow *vwindow)
 : BC_Window(MWindow::create_title(N_("Viewer")),
	mainsession->vwindow_x,
	mainsession->vwindow_y,
	mainsession->vwindow_w,
	mainsession->vwindow_h,
	100,
	100,
	1,
	1,
	1)
{
	this->vwindow = vwindow;
	loaded_title[0] = 0;

	set_icon(vwindow->get_window_icon());

	theme_global->get_vwindow_sizes(this);
	theme_global->draw_vwindow_bg(this);
	flash();

	meters = new VWindowMeters(this,
		theme_global->vmeter_x,
		theme_global->vmeter_y,
		theme_global->vmeter_h);

// Requires meters to build
	edit_panel = new VWindowEditing(vwindow, meters, this);

	add_subwindow(slider = new VWindowSlider(vwindow,
		this,
		theme_global->vslider_x,
		theme_global->vslider_y,
		theme_global->vslider_w));

	transport = new VWindowTransport(this,
		theme_global->vtransport_x,
		theme_global->vtransport_y);

	add_subwindow(clock = new MainClock(
		theme_global->vtime_x,
		theme_global->vtime_y,
		theme_global->vtime_w));

	canvas = new VWindowCanvas(this);

	add_subwindow(timebar = new VTimeBar(this,
		theme_global->vtimebar_x,
		theme_global->vtimebar_y,
		theme_global->vtimebar_w,
		theme_global->vtimebar_h));
	timebar->update();

	deactivate();
	slider->activate();
	resize_event(mainsession->vwindow_w, mainsession->vwindow_h);
}

VWindowGUI::~VWindowGUI()
{
	delete canvas;
	delete transport;
}

void VWindowGUI::change_source(const char *title)
{
	int no_title = 0;

	if(title && title[0])
		sprintf(loaded_title,"%s - " PROGRAM_NAME, title);
	else
	{
		strcpy(loaded_title, MWindow::create_title(N_("Viewer")));
		no_title = 1;
	}
	slider->set_position();
	timebar->update();
	set_title(loaded_title);
	if(no_title)
		loaded_title[0] = 0;
}

void VWindowGUI::resize_event(int w, int h)
{
	mainsession->vwindow_location(-1, -1, w, h);
	theme_global->get_vwindow_sizes(this);
	theme_global->draw_vwindow_bg(this);
	flash();

	edit_panel->reposition_buttons(theme_global->vedit_x,
		theme_global->vedit_y);
	slider->reposition_window(theme_global->vslider_x,
		theme_global->vslider_y,
		theme_global->vslider_w);
// Recalibrate pointer motion range
	slider->set_position();
	timebar->resize_event();
	transport->reposition_buttons(theme_global->vtransport_x,
		theme_global->vtransport_y);
	clock->reposition_window(theme_global->vtime_x,
		theme_global->vtime_y,
		theme_global->vtime_w);
	canvas->reposition_window(0,
		theme_global->vcanvas_x,
		theme_global->vcanvas_y,
		theme_global->vcanvas_w,
		theme_global->vcanvas_h);
	meters->reposition_window(theme_global->vmeter_x,
		theme_global->vmeter_y,
		theme_global->vmeter_h);
}

void VWindowGUI::translation_event()
{
	if(mainsession->vwindow_location(get_x(), get_y(), get_w(), get_h()))
	{
		reposition_window(mainsession->vwindow_x,
			mainsession->vwindow_y,
			mainsession->vwindow_w,
			mainsession->vwindow_h);
		resize_event(mainsession->vwindow_w,
			mainsession->vwindow_h);
	}
}

void VWindowGUI::close_event()
{
	vwindow->playback_engine->send_command(STOP);
	hide_window();
	mwindow_global->mark_vwindow_hidden();
}

int VWindowGUI::keypress_event()
{
	int result = 0;

	switch(get_keypress())
	{
	case 'w':
	case 'W':
		close_event();
		result = 1;
		break;
	case 'z':
		mwindow_global->undo_entry();
		break;
	case 'Z':
		mwindow_global->redo_entry();
		break;
	case 'f':
		if(mainsession->vwindow_fullscreen)
			canvas->stop_fullscreen();
		else
			canvas->start_fullscreen();
		break;
	case ESC:
		if(mainsession->vwindow_fullscreen)
			canvas->stop_fullscreen();
		break;
	}
	if(!result) result = transport->keypress_event();

	return result;
}

int VWindowGUI::button_press_event()
{
	if(canvas->get_canvas())
		return canvas->button_press_event_base(canvas->get_canvas());
	return 0;
}

void VWindowGUI::cursor_leave_event()
{
	if(canvas->get_canvas())
		canvas->cursor_leave_event_base(canvas->get_canvas());
}

int VWindowGUI::cursor_enter_event()
{
	if(canvas->get_canvas())
		return canvas->cursor_enter_event_base(canvas->get_canvas());
	return 0;
}

int VWindowGUI::button_release_event()
{
	if(canvas->get_canvas())
		return canvas->button_release_event();
	return 0;
}

int VWindowGUI::cursor_motion_event()
{
	if(canvas->get_canvas())
	{
		canvas->get_canvas()->unhide_cursor();
		return canvas->cursor_motion_event();
	}
	return 0;
}

void VWindowGUI::drag_motion()
{
	int cursor_x, cursor_y;

	if(get_hidden() || mainsession->current_operation != DRAG_ASSET)
		return;

	int old_status = mainsession->vcanvas_highlighted;

	if(get_cursor_over_window(&cursor_x, &cursor_y))
	{
		mainsession->vcanvas_highlighted =
			cursor_x >= canvas->x &&
			cursor_x < canvas->x + canvas->w &&
			cursor_y >= canvas->y &&
			cursor_y < canvas->y + canvas->h;
	}
	else
		mainsession->vcanvas_highlighted = 0;

	if(old_status != mainsession->vcanvas_highlighted)
		canvas->draw_refresh();
}

void VWindowGUI::drag_stop()
{
	if(get_hidden())
		return;

	if(mainsession->vcanvas_highlighted &&
		mainsession->current_operation == DRAG_ASSET)
	{
		mainsession->vcanvas_highlighted = 0;
		canvas->draw_refresh();

		Asset *asset = mainsession->drag_assets->total ?
			mainsession->drag_assets->values[0] : 0;
		EDL *edl = mainsession->drag_clips->total ?
			mainsession->drag_clips->values[0] : 0;

		if(asset)
			vwindow->change_source(asset);
		else
		if(edl)
			vwindow->change_source(edl);
		return;
	}
}


VWindowMeters::VWindowMeters(VWindowGUI *gui,
	int x, 
	int y, 
	int h)
 : MeterPanel(gui, x, y, h,
	edlsession->audio_channels, edlsession->vwindow_meter)
{
	this->gui = gui;
}

int VWindowMeters::change_status_event()
{
	edlsession->vwindow_meter = use_meters;
	theme_global->get_vwindow_sizes(gui);
	gui->resize_event(gui->get_w(), gui->get_h());
	return 1;
}


VWindowEditing::VWindowEditing(VWindow *vwindow,
	MeterPanel *meter_panel, VWindowGUI *gui)
 : EditPanel(gui,
	theme_global->vedit_x,
	theme_global->vedit_y,
	EDTP_SPLICE | EDTP_OVERWRITE | EDTP_COPY | EDTP_LABELS | EDTP_TOCLIP,
	meter_panel)
{
	this->vwindow = vwindow;
}

void VWindowEditing::copy_selection()
{
	vwindow->copy();
}

void VWindowEditing::splice_selection()
{
	mwindow_global->splice(vwindow_edl);
}

void VWindowEditing::overwrite_selection()
{
	mwindow_global->overwrite(vwindow_edl);
}

void VWindowEditing::toggle_label()
{
	vwindow_edl->labels->toggle_label(vwindow_edl->local_session->get_selectionstart(1),
		vwindow_edl->local_session->get_selectionend(1));
	vwindow->gui->timebar->update();
}

void VWindowEditing::prev_label()
{
	vwindow->playback_engine->interrupt_playback();

	Label *current = vwindow_edl->labels->prev_label(
			vwindow_edl->local_session->get_selectionstart(1));

	if(!current)
	{
		vwindow_edl->local_session->set_selection(0);
		vwindow->update_position(0, 1);
	}
	else
	{
		vwindow_edl->local_session->set_selection(current->position);
		vwindow->update_position(0, 1);
	}
}

void VWindowEditing::next_label()
{
	Label *current = vwindow_edl->labels->next_label(
		vwindow_edl->local_session->get_selectionstart(1));
	vwindow->playback_engine->interrupt_playback();

	if(!current)
	{
		ptstime position = vwindow_edl->duration();

		vwindow_edl->local_session->set_selection(position);
		vwindow->update_position(0, 1);
	}
	else
	{
		vwindow_edl->local_session->set_selection(current->position);
		vwindow->update_position(0, 1);
	}
}

void VWindowEditing::set_inpoint()
{
	vwindow->set_inpoint();
}

void VWindowEditing::set_outpoint()
{
	vwindow->set_outpoint();
}

void VWindowEditing::to_clip()
{
	EDL *new_edl = new EDL(0);
	ptstime start = vwindow_edl->local_session->get_selectionstart();
	ptstime end = vwindow_edl->local_session->get_selectionend();
	char string[BCTEXTLEN];

	if(PTSEQU(start, end))
	{
		end = vwindow_edl->duration();
		start = 0;
	}

	new_edl->copy(vwindow_edl, start, end);
	sprintf(new_edl->local_session->clip_title, _("Clip %d"),
		mainsession->clip_number++);
	edlsession->ptstotext(string, end - start);
	sprintf(new_edl->local_session->clip_notes, _("%s\nCreated from:\n%s"), string, vwindow->gui->loaded_title);

	new_edl->local_session->set_selection(0);
	mwindow_global->clip_edit->create_clip(new_edl);
}


VWindowSlider::VWindowSlider(VWindow *vwindow,
	VWindowGUI *gui,
	int x, 
	int y, 
	int pixels)
 : BC_PercentageSlider(x, 
			y,
			0,
			pixels, 
			pixels, 
			0, 
			0, 
			0)
{
	this->vwindow = vwindow;
	this->gui = gui;
	set_precision(0.00001);
	set_pagination(1.0, 10.0);
}

int VWindowSlider::handle_event()
{
	vwindow->playback_engine->interrupt_playback();

	vwindow->update_position(1, 0);
	return 1;
}

void VWindowSlider::set_position()
{
	ptstime new_length = vwindow_edl->duration();

	if(EQUIV(vwindow_edl->local_session->preview_end, 0))
		vwindow_edl->local_session->preview_end = new_length;
	if(vwindow_edl->local_session->preview_end > new_length)
		vwindow_edl->local_session->preview_end = new_length;
	if(vwindow_edl->local_session->preview_start > new_length)
		vwindow_edl->local_session->preview_start = 0;

	update(theme_global->vslider_w,
		vwindow_edl->local_session->get_selectionstart(1),
		vwindow_edl->local_session->preview_start,
		vwindow_edl->local_session->preview_end);
}


VWindowTransport::VWindowTransport(VWindowGUI *gui,
	int x, 
	int y)
 : PlayTransport(gui, x, y)
{
	this->gui = gui;
}

void VWindowTransport::goto_start()
{
	handle_transport(REWIND, 1);
	gui->vwindow->goto_start();
}

void VWindowTransport::goto_end()
{
	handle_transport(GOTO_END, 1);
	gui->vwindow->goto_end();
}


VWindowCanvas::VWindowCanvas(VWindowGUI *gui)
 : Canvas(0,
	gui,
	theme_global->vcanvas_x,
	theme_global->vcanvas_y,
	theme_global->vcanvas_w,
	theme_global->vcanvas_h,
	0)
{
	this->gui = gui;
}

void VWindowCanvas::zoom_resize_window(double percentage)
{
	int canvas_w, canvas_h;
	int new_w, new_h;

	calculate_sizes(edlsession->output_w,
		edlsession->output_h,
		percentage,
		canvas_w,
		canvas_h);
	new_w = canvas_w + (gui->get_w() - theme_global->vcanvas_w);
	new_h = canvas_h + (gui->get_h() - theme_global->vcanvas_h);
	gui->resize_window(new_w, new_h);
	gui->resize_event(new_w, new_h);
}

void VWindowCanvas::close_source()
{
	gui->vwindow->playback_engine->interrupt_playback();
	gui->vwindow->remove_source();
}

void VWindowCanvas::draw_overlays()
{
	if(mainsession->vcanvas_highlighted)
	{
		get_canvas()->set_color(WHITE);
		get_canvas()->set_inverse();
		get_canvas()->draw_rectangle(0, 0, get_canvas()->get_w(), get_canvas()->get_h());
		get_canvas()->draw_rectangle(1, 1, get_canvas()->get_w() - 2, get_canvas()->get_h() - 2);
		get_canvas()->set_opaque();
	}
}

int VWindowCanvas::get_fullscreen()
{
	return mainsession->vwindow_fullscreen;
}

void VWindowCanvas::set_fullscreen(int value)
{
	mainsession->vwindow_fullscreen = value;
}

double VWindowCanvas::sample_aspect_ratio()
{
	Asset *asset = 0;

	if(vwindow_edl->this_edlsession)
		return vwindow_edl->this_edlsession->sample_aspect_ratio;
	return 1.0;
}
