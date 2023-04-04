// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "automation.h"
#include "autos.h"
#include "bcpixmap.h"
#include "bcsignals.h"
#include "bcresources.h"
#include "cinelerra.h"
#include "canvas.h"
#include "clip.h"
#include "cpanel.h"
#include "cplayback.h"
#include "colormodels.inc"
#include "cropauto.h"
#include "cropautos.h"
#include "ctimebar.h"
#include "cursors.h"
#include "cwindowgui.h"
#include "cwindow.h"
#include "cwindowtool.h"
#include "editpanel.h"
#include "edl.h"
#include "edlsession.h"
#include "floatauto.h"
#include "floatautos.h"
#include "guidelines.h"
#include "keys.h"
#include "language.h"
#include "localsession.h"
#include "mainclock.h"
#include "mainmenu.h"
#include "mainundo.h"
#include "mainsession.h"
#include "maskauto.h"
#include "maskautos.h"
#include "mbuttons.h"
#include "meterpanel.h"
#include "mwindow.h"
#include "playtransport.h"
#include "theme.h"
#include "trackcanvas.h"
#include "tracks.h"
#include "vtrack.h"
#include "vframe.h"


static double my_zoom_table[] =
{
	0.25,
	0.33,
	0.50,
	0.75,
	1.0,
	1.5,
	2.0,
	3.0,
	4.0
};

static int total_zooms = sizeof(my_zoom_table) / sizeof(double);


CWindowGUI::CWindowGUI(CWindow *cwindow)
 : BC_Window(MWindow::create_title(N_("Compositor")),
	mainsession->cwindow_x, mainsession->cwindow_y,
	mainsession->cwindow_w, mainsession->cwindow_h,
	100, 100, 1, 1, 1,
	BC_WindowBase::get_resources()->bg_color,
	edlsession->get_cwindow_display())
{
	this->cwindow = cwindow;
	affected_track = 0;
	affected_x = 0;
	affected_y = 0;
	affected_z = 0;
	affected_keyframe = 0;
	affected_point = 0;
	x_offset = 0;
	y_offset = 0;
	x_origin = 0;
	y_origin = 0;
	current_operation = CWINDOW_NONE;
	translating_zoom = 0;
	active = 0;
	inactive = 0;
	crop_translate = 0;

	set_icon(theme_global->get_image("cwindow_icon"));

	active = new BC_Pixmap(this, theme_global->get_image("cwindow_active"));
	inactive = new BC_Pixmap(this, theme_global->get_image("cwindow_inactive"));

	theme_global->get_cwindow_sizes(this, mainsession->cwindow_controls);
	theme_global->draw_cwindow_bg(this);
	flash();

// Meters required by composite panel
	meters = new CWindowMeters(this, theme_global->cmeter_x,
		theme_global->cmeter_y, theme_global->cmeter_h);

	composite_panel = new CPanel(this, theme_global->ccomposite_x,
		theme_global->ccomposite_y, theme_global->ccomposite_w,
		theme_global->ccomposite_h);

	canvas = new CWindowCanvas(this);

	add_subwindow(timebar = new CTimeBar(this,
		theme_global->ctimebar_x, theme_global->ctimebar_y,
		theme_global->ctimebar_w, theme_global->ctimebar_h));
	timebar->update();

	add_subwindow(slider = new CWindowSlider(cwindow,
		theme_global->cslider_x, theme_global->cslider_y,
		theme_global->cslider_w));

	transport = new CWindowTransport(this,
		theme_global->ctransport_x, theme_global->ctransport_y);

	edit_panel = new CWindowEditing(this, meters);

	zoom_panel = new CWindowZoom(this, theme_global->czoom_x,
		theme_global->czoom_y, _("Auto"));

	if(!edlsession->cwindow_scrollbars)
		zoom_panel->update(_("Auto"));

// Must create after meter panel
	tool_panel = new CWindowTool(this);
	tool_panel->Thread::start();

	set_operation(edlsession->cwindow_operation);
	resize_event(mainsession->cwindow_w, mainsession->cwindow_h);
}

CWindowGUI::~CWindowGUI()
{
	delete tool_panel;
	delete meters;
	delete composite_panel;
	delete canvas;
	delete transport;
	delete edit_panel;
	delete zoom_panel;
	delete active;
	delete inactive;
}

void CWindowGUI::translation_event()
{
	if(mainsession->cwindow_location(get_x(), get_y(), get_w(), get_h()))
	{
		reposition_window(mainsession->cwindow_x, mainsession->cwindow_y,
			mainsession->cwindow_w, mainsession->cwindow_h);
		resize_event(mainsession->cwindow_w,
			mainsession->cwindow_h);
	}
}

void CWindowGUI::resize_event(int w, int h)
{
	mainsession->cwindow_location(-1, -1, w, h);

	theme_global->get_cwindow_sizes(this, mainsession->cwindow_controls);
	theme_global->draw_cwindow_bg(this);
	flash();

	composite_panel->reposition_buttons(theme_global->ccomposite_x,
		theme_global->ccomposite_y);

	canvas->reposition_window(master_edl,
		theme_global->ccanvas_x, theme_global->ccanvas_y,
		theme_global->ccanvas_w, theme_global->ccanvas_h);
	canvas->update_guidelines();
	timebar->resize_event();

	slider->reposition_window(theme_global->cslider_x,
		theme_global->cslider_y, theme_global->cslider_w);
// Recalibrate pointer motion range
	slider->set_position();

	transport->reposition_buttons(theme_global->ctransport_x,
		theme_global->ctransport_y);

	edit_panel->reposition_buttons(theme_global->cedit_x,
		theme_global->cedit_y);

	zoom_panel->reposition_window(theme_global->czoom_x,
		theme_global->czoom_y);

	meters->reposition_window(theme_global->cmeter_x,
		theme_global->cmeter_y, theme_global->cmeter_h);

	draw_status();
}

int CWindowGUI::button_press_event()
{
	BC_WindowBase *win;

	if(win = canvas->get_canvas())
		return canvas->button_press_event_base(win);
	return 0;
}

void CWindowGUI::cursor_leave_event()
{
	BC_WindowBase *win;

	if(win = canvas->get_canvas())
		canvas->cursor_leave_event_base(win);
}

int CWindowGUI::cursor_enter_event()
{
	BC_WindowBase *win;

	if(win = canvas->get_canvas())
		return canvas->cursor_enter_event_base(win);
	return 0;
}

int CWindowGUI::button_release_event()
{
	if(canvas->get_canvas())
		return canvas->button_release_event();
	return 0;
}

int CWindowGUI::cursor_motion_event()
{
	if(canvas->get_canvas())
	{
		canvas->get_canvas()->unhide_cursor();
		return canvas->cursor_motion_event();
	}
	return 0;
}

void CWindowGUI::draw_status()
{
	if(canvas->get_canvas() && 
			canvas->get_canvas()->get_video_on() ||
			canvas->is_processing)
		draw_pixmap(active, theme_global->cstatus_x,
			theme_global->cstatus_y);
	else
		draw_pixmap(inactive, theme_global->cstatus_x,
			theme_global->cstatus_y);

	flash(theme_global->cstatus_x, theme_global->cstatus_y,
		active->get_w(), active->get_h());
}

void CWindowGUI::zoom_canvas(int do_auto, double value, int update_menu)
{
	double x, y;
	double w, h;
	int old_auto = edlsession->cwindow_scrollbars;
	double new_zoom = value;
	double old_zoom = edlsession->cwindow_zoom;

	if(do_auto)
	{
		edlsession->cwindow_scrollbars = 0;
		new_zoom = old_zoom;
		x = canvas->get_xscroll();
		y = canvas->get_yscroll();
	}
	else
	{
		edlsession->cwindow_scrollbars = 1;

		master_edl->calculate_conformed_dimensions(w, h);

		x = y = 0;
		if(canvas->h_visible < (int)round(h * new_zoom))
			x = canvas->get_xscroll() * new_zoom / old_zoom;

		if(canvas->w_visible < (int)round(w * new_zoom))
			y = canvas->get_yscroll() * new_zoom / old_zoom;
	}

	if(update_menu)
	{
		if(do_auto)
			zoom_panel->update(_("Auto"));
		else
			zoom_panel->update(value);
	}
	canvas->update_zoom(x, y, new_zoom);
	canvas->reposition_window(master_edl,
		theme_global->ccanvas_x, theme_global->ccanvas_y,
		theme_global->ccanvas_w, theme_global->ccanvas_h);
	canvas->draw_refresh();
}

// TODO
// Don't refresh the canvas in a load file operation which is going to
// refresh it anyway.
void CWindowGUI::set_operation(int value)
{
	edlsession->cwindow_operation = value;

	composite_panel->set_operation(value);
	edit_panel->update();

	tool_panel->start_tool(value);
	canvas->draw_refresh();
}

void CWindowGUI::update_tool()
{
	tool_panel->update_values();
}

void CWindowGUI::close_event()
{
	cwindow->hide_window();
}

int CWindowGUI::keypress_event()
{
	int result = 0;

	switch(get_keypress())
	{
	case 'w':
	case 'W':
		close_event();
		result = 1;
		break;
	case '+':
	case '=':
		keyboard_zoomin();
		result = 1;
		break;
	case '-':
		keyboard_zoomout();
		result = 1;
		break;
	case 'f':
		if(mainsession->cwindow_fullscreen)
			canvas->stop_fullscreen();
		else
			canvas->start_fullscreen();
		break;
	case ESC:
		if(mainsession->cwindow_fullscreen)
			canvas->stop_fullscreen();
		break;
	case LEFT:
		if(!ctrl_down())
		{
			if(alt_down())
			{
				int shift_down = this->shift_down();

				mwindow_global->stop_composer();
				mwindow_global->prev_edit_handle(shift_down);
			}
			else
				mwindow_global->move_left();

			result = 1;
		}
		break;
	case RIGHT:
		if(!ctrl_down())
		{
			if(alt_down())
			{
				int shift_down = this->shift_down();

				mwindow_global->stop_composer();
				mwindow_global->next_edit_handle(shift_down);
			}
			else
				mwindow_global->move_right();
			result = 1;
		}
		break;
	}

	if(!result)
		result = transport->keypress_event();

	return result;
}

void CWindowGUI::reset_affected()
{
	affected_x = 0;
	affected_y = 0;
	affected_z = 0;
}

void CWindowGUI::keyboard_zoomin()
{
	zoom_panel->zoom_tumbler->handle_up_event();
}

void CWindowGUI::keyboard_zoomout()
{
	zoom_panel->zoom_tumbler->handle_down_event();
}

void CWindowGUI::drag_motion()
{
	if(get_hidden())
		return;

	if(mainsession->current_operation == DRAG_ASSET ||
		mainsession->current_operation == DRAG_VTRANSITION ||
		mainsession->current_operation == DRAG_VEFFECT)
	{
		int old_status = mainsession->ccanvas_highlighted;
		int cursor_x, cursor_y;

		if(get_cursor_over_window(&cursor_x, &cursor_y))
			mainsession->ccanvas_highlighted =
				cursor_x >= canvas->x &&
				cursor_x < canvas->x + canvas->w &&
				cursor_y >= canvas->y &&
				cursor_y < canvas->y + canvas->h;
		else
			mainsession->ccanvas_highlighted = 0;

		if(old_status != mainsession->ccanvas_highlighted)
			canvas->draw_refresh();
	}
}

void CWindowGUI::drag_stop()
{
	if(get_hidden())
		return;

	if((mainsession->current_operation == DRAG_ASSET ||
		mainsession->current_operation == DRAG_VTRANSITION ||
		mainsession->current_operation == DRAG_VEFFECT) &&
		mainsession->ccanvas_highlighted)
	{
// Hide highlighting
		mainsession->ccanvas_highlighted = 0;
		canvas->draw_refresh();
	}
	else
		return;

	if(mainsession->current_operation == DRAG_ASSET)
	{
		if(mainsession->drag_assets->total)
		{
			mwindow_global->clear(0);
			mwindow_global->load_assets(mainsession->drag_assets,
				master_edl->local_session->get_selectionstart(),
				mainsession->track_highlighted,
				0); // overwrite
		}

		if(mainsession->drag_clips->total)
		{
			mwindow_global->clear(0);
			mwindow_global->paste_edls(mainsession->drag_clips,
				LOADMODE_PASTE, 
				mainsession->track_highlighted,
				master_edl->local_session->get_selectionstart(),
				0); // overwrite
		}

		if(mainsession->drag_assets->total ||
			mainsession->drag_clips->total)
		{
			mwindow_global->save_backup();
			mwindow_global->update_gui(WUPD_SCROLLBARS | WUPD_CANVINCR |
				WUPD_TIMEBAR | WUPD_ZOOMBAR | WUPD_CLOCK);
			mwindow_global->undo->update_undo(_("insert assets"), LOAD_ALL);
			mwindow_global->sync_parameters();
		}
	}

	if(mainsession->current_operation == DRAG_VEFFECT)
	{
		Track *affected_track = cwindow->calculate_affected_track();

		mwindow_global->insert_effects_cwindow(affected_track);
		mainsession->current_operation = NO_OPERATION;
	}

	if(mainsession->current_operation == DRAG_VTRANSITION)
	{
		mwindow_global->paste_transition(STRDSC_VIDEO,
			mainsession->drag_pluginservers->values[0]);
		mainsession->current_operation = NO_OPERATION;
	}
}

void CWindowGUI::clear_canvas()
{
	for(Track *current = master_edl->tracks->first; current; current = current->next)
	{
		if(current->camera_gframe)
			current->camera_gframe->set_enabled(0);
		if(current->projector_gframe)
			current->projector_gframe->set_enabled(0);
	}
	canvas->safe_regions->set_enabled(0);
	canvas->clear_canvas();
}


CWindowEditing::CWindowEditing(CWindowGUI *gui, MeterPanel *meter_panel)
 : EditPanel(gui, theme_global->cedit_x, theme_global->cedit_y,
	EDTP_KEYFRAME | EDTP_COPY | EDTP_PASTE | EDTP_UNDO |
		EDTP_LABELS | EDTP_TOCLIP | EDTP_CUT,
	meter_panel)
{
}

void CWindowEditing::set_inpoint()
{
	mwindow_global->set_inpoint();
}

void CWindowEditing::set_outpoint()
{
	mwindow_global->set_outpoint();
}


CWindowMeters::CWindowMeters(CWindowGUI *gui, int x, int y, int h)
 : MeterPanel(gui, x, y, h, edlsession->audio_channels, edlsession->cwindow_meter)
{
	this->gui = gui;
}

int CWindowMeters::change_status_event()
{
	edlsession->cwindow_meter = use_meters;
	theme_global->get_cwindow_sizes(gui, mainsession->cwindow_controls);
	gui->resize_event(gui->get_w(), gui->get_h());
	return 1;
}


CWindowZoom::CWindowZoom(CWindowGUI *gui, int x, int y,
	const char *first_item_text)
 : ZoomPanel(gui, edlsession->cwindow_zoom, x, y, 80,  my_zoom_table,
	total_zooms, ZOOM_PERCENTAGE, first_item_text)
{
	this->gui = gui;
}

int CWindowZoom::handle_event()
{
	if(!strcasecmp(_("Auto"), get_text()))
		gui->zoom_canvas(1, get_value(), 0);
	else
		gui->zoom_canvas(0, get_value(), 0);
	return 1;
}


CWindowSlider::CWindowSlider(CWindow *cwindow, int x, int y, int pixels)
 : BC_PercentageSlider(x, y, 0, pixels, pixels, 0, 1, 0)
{
	this->cwindow = cwindow;
	set_precision(0.00001);
	set_pagination(1.0, 10.0);
}

int CWindowSlider::handle_event()
{
	cwindow->playback_engine->interrupt_playback();

	mwindow_global->select_point(get_value());
	return 1;
}

void CWindowSlider::set_position()
{
	ptstime new_length = master_edl->duration();

	if(master_edl->local_session->preview_end <= 0 ||
			master_edl->local_session->preview_end > new_length)
		master_edl->local_session->preview_end = new_length;
	if(master_edl->local_session->preview_start >
			master_edl->local_session->preview_end)
		master_edl->local_session->preview_start = 0;

	update(theme_global->cslider_w,
		master_edl->local_session->get_selectionstart(1),
		master_edl->local_session->preview_start,
		master_edl->local_session->preview_end);
}

void CWindowSlider::increase_value()
{
	cwindow->gui->transport->handle_transport(SINGLE_FRAME_FWD);
}

void CWindowSlider::decrease_value()
{
	cwindow->gui->transport->handle_transport(SINGLE_FRAME_REWIND);
}


CWindowTransport::CWindowTransport(CWindowGUI *gui, int x, int y)
 : PlayTransport(gui, x, y)
{
}

void CWindowTransport::goto_start()
{
	handle_transport(REWIND, 1);
	mwindow_global->goto_start();
}

void CWindowTransport::goto_end()
{
	handle_transport(GOTO_END, 1);
	mwindow_global->goto_end();
}


CWindowCanvas::CWindowCanvas(CWindowGUI *gui)
 : Canvas(gui, 0, theme_global->ccanvas_x, theme_global->ccanvas_y,
	theme_global->ccanvas_w, theme_global->ccanvas_h,
	edlsession->cwindow_scrollbars)
{
	this->gui = gui;
	safe_regions = guidelines.append_frame(0, MAX_PTSTIME);
	update_guidelines();
}

void CWindowCanvas::status_event()
{
	gui->draw_status();
}

int CWindowCanvas::get_fullscreen()
{
	return mainsession->cwindow_fullscreen;
}

void CWindowCanvas::set_fullscreen(int value)
{
	mainsession->cwindow_fullscreen = value;
}

void CWindowCanvas::update_zoom(int x, int y, double zoom)
{
	use_scrollbars = edlsession->cwindow_scrollbars;

	edlsession->cwindow_xscroll = x;
	edlsession->cwindow_yscroll = y;
	edlsession->cwindow_zoom = zoom;
}

void CWindowCanvas::zoom_auto()
{
	gui->zoom_canvas(1, 1.0, 1);
}

int CWindowCanvas::get_xscroll()
{
	return edlsession->cwindow_xscroll;
}

int CWindowCanvas::get_yscroll()
{
	return edlsession->cwindow_yscroll;
}

double CWindowCanvas::get_zoom()
{
	return edlsession->cwindow_zoom;
}

#define CROPHANDLE_W 10
#define CROPHANDLE_H 10

#define CONTROL_W 10
#define CONTROL_H 10
#define FIRST_CONTROL_W 20
#define FIRST_CONTROL_H 20

#define RULERHANDLE_W 16
#define RULERHANDLE_H 16

int CWindowCanvas::do_ruler(int draw,
	int motion,
	int button_press,
	int button_release)
{
	int result = 0;
	double x1 = edlsession->ruler_x1;
	double y1 = edlsession->ruler_y1;
	double x2 = edlsession->ruler_x2;
	double y2 = edlsession->ruler_y2;
	double canvas_x1 = x1;
	double canvas_y1 = y1;
	double canvas_x2 = x2;
	double canvas_y2 = y2;
	double output_x = get_cursor_x();
	double output_y = get_cursor_y();
	double canvas_cursor_x = output_x;
	double canvas_cursor_y = output_y;
	double old_x1 = x1;
	double old_x2 = x2;
	double old_y1 = y1;
	double old_y2 = y2;

	canvas_to_output(output_x, output_y);
	output_to_canvas(canvas_x1, canvas_y1);
	output_to_canvas(canvas_x2, canvas_y2);
	mainsession->cwindow_output_x = round(output_x);
	mainsession->cwindow_output_y = round(output_y);

	if(button_press && get_buttonpress() == 1)
	{
		gui->ruler_handle = -1;
		gui->ruler_translate = 0;

		if(gui->alt_down())
		{
			gui->ruler_translate = 1;
			gui->ruler_origin_x = x1;
			gui->ruler_origin_y = y1;
		}
		else
		if(canvas_cursor_x >= canvas_x1 - RULERHANDLE_W / 2 &&
			canvas_cursor_x < canvas_x1 + RULERHANDLE_W / 2 &&
			canvas_cursor_y >= canvas_y1 - RULERHANDLE_W &&
			canvas_cursor_y < canvas_y1 + RULERHANDLE_H / 2)
		{
			gui->ruler_handle = 0;
			gui->ruler_origin_x = x1;
			gui->ruler_origin_y = y1;
		}
		else
		if(canvas_cursor_x >= canvas_x2 - RULERHANDLE_W / 2 &&
			canvas_cursor_x < canvas_x2 + RULERHANDLE_W / 2 &&
			canvas_cursor_y >= canvas_y2 - RULERHANDLE_W &&
			canvas_cursor_y < canvas_y2 + RULERHANDLE_H / 2)
		{
			gui->ruler_handle = 1;
			gui->ruler_origin_x = x2;
			gui->ruler_origin_y = y2;
		}

// Start new selection
		if(!gui->ruler_translate &&
			(gui->ruler_handle < 0 ||
			(EQUIV(x2, x1) && EQUIV(y2, y1))))
		{
// Hide previous
			do_ruler(1, 0, 0, 0);
			get_canvas()->flash();
			gui->ruler_handle = 1;
			edlsession->ruler_x1 = output_x;
			edlsession->ruler_y1 = output_y;
			edlsession->ruler_x2 = output_x;
			edlsession->ruler_y2 = output_y;
			gui->ruler_origin_x = edlsession->ruler_x2;
			gui->ruler_origin_y = edlsession->ruler_y2;
		}

		gui->x_origin = output_x;
		gui->y_origin = output_y;
		gui->current_operation = CWINDOW_RULER;
		gui->tool_panel->raise_window();
		result = 1;
	}

	if(motion)
	{
		if(gui->current_operation == CWINDOW_RULER)
		{
			if(gui->ruler_translate)
			{
// Hide ruler
				do_ruler(1, 0, 0, 0);
				double x_difference = edlsession->ruler_x1;
				double y_difference = edlsession->ruler_y1;
				edlsession->ruler_x1 =
					output_x - gui->x_origin + gui->ruler_origin_x;
				edlsession->ruler_y1 =
					output_y - gui->y_origin + gui->ruler_origin_y;
				x_difference -= edlsession->ruler_x1;
				y_difference -= edlsession->ruler_y1;
				edlsession->ruler_x2 -= x_difference;
				edlsession->ruler_y2 -= y_difference;
// Show ruler
				do_ruler(1, 0, 0, 0);
				get_canvas()->flash();
			}
			else
			switch(gui->ruler_handle)
			{
			case 0:
				do_ruler(1, 0, 0, 0);
				edlsession->ruler_x1 =
					output_x - gui->x_origin + gui->ruler_origin_x;
				edlsession->ruler_y1 =
					output_y - gui->y_origin + gui->ruler_origin_y;

				if(gui->alt_down() || gui->ctrl_down())
				{
					double angle_value = fabs(atan((edlsession->ruler_y2 - edlsession->ruler_y1) /
						(edlsession->ruler_x2 - edlsession->ruler_x1)) *
							360 / 2 / M_PI);
					double distance_value =
						sqrt(SQR(edlsession->ruler_x2 - edlsession->ruler_x1) +
						SQR(edlsession->ruler_y2 - edlsession->ruler_y1));
					if(angle_value < 22)
						edlsession->ruler_y1 = edlsession->ruler_y2;
					else
					if(angle_value > 67)
						edlsession->ruler_x1 = edlsession->ruler_x2;
					else
					if(edlsession->ruler_x1 < edlsession->ruler_x2 &&
						edlsession->ruler_y1 < edlsession->ruler_y2)
					{
						edlsession->ruler_x1 = edlsession->ruler_x2 - distance_value / 1.414214;
						edlsession->ruler_y1 = edlsession->ruler_y2 - distance_value / 1.414214;
					}
					else
					if(edlsession->ruler_x1 < edlsession->ruler_x2 &&
						edlsession->ruler_y1 > edlsession->ruler_y2)
					{
						edlsession->ruler_x1 = edlsession->ruler_x2 - distance_value / 1.414214;
						edlsession->ruler_y1 = edlsession->ruler_y2 + distance_value / 1.414214;
					}
					else
					if(edlsession->ruler_x1 > edlsession->ruler_x2 &&
						edlsession->ruler_y1 < edlsession->ruler_y2)
					{
						edlsession->ruler_x1 = edlsession->ruler_x2 + distance_value / 1.414214;
						edlsession->ruler_y1 = edlsession->ruler_y2 - distance_value / 1.414214;
					}
					else
					{
						edlsession->ruler_x1 = edlsession->ruler_x2 + distance_value / 1.414214;
						edlsession->ruler_y1 = edlsession->ruler_y2 + distance_value / 1.414214;
					}
				}
				do_ruler(1, 0, 0, 0);
				get_canvas()->flash();
				gui->update_tool();
				break;

			case 1:
				do_ruler(1, 0, 0, 0);
				edlsession->ruler_x2 = output_x - gui->x_origin + gui->ruler_origin_x;
				edlsession->ruler_y2 = output_y - gui->y_origin + gui->ruler_origin_y;

				if(gui->alt_down() || gui->ctrl_down())
				{
					double angle_value = fabs(atan((edlsession->ruler_y2 - edlsession->ruler_y1) /
						(edlsession->ruler_x2 - edlsession->ruler_x1)) *
							360 / 2 / M_PI);
					double distance_value =
						sqrt(SQR(edlsession->ruler_x2 - edlsession->ruler_x1) +
							SQR(edlsession->ruler_y2 - edlsession->ruler_y1));
					if(angle_value < 22)
						edlsession->ruler_y2 = edlsession->ruler_y1;
					else
					if(angle_value > 67)
						edlsession->ruler_x2 = edlsession->ruler_x1;
					else
					if(edlsession->ruler_x2 < edlsession->ruler_x1 &&
						edlsession->ruler_y2 < edlsession->ruler_y1)
					{
						edlsession->ruler_x2 = edlsession->ruler_x1 - distance_value / 1.414214;
						edlsession->ruler_y2 = edlsession->ruler_y1 - distance_value / 1.414214;
					}
					else
					if(edlsession->ruler_x2 < edlsession->ruler_x1 &&
						edlsession->ruler_y2 > edlsession->ruler_y1)
					{
						edlsession->ruler_x2 = edlsession->ruler_x1 - distance_value / 1.414214;
						edlsession->ruler_y2 = edlsession->ruler_y1 + distance_value / 1.414214;
					}
					else
					if(edlsession->ruler_x2 > edlsession->ruler_x1 &&
						edlsession->ruler_y2 < edlsession->ruler_y1)
					{
						edlsession->ruler_x2 = edlsession->ruler_x1 + distance_value / 1.414214;
						edlsession->ruler_y2 = edlsession->ruler_y1 - distance_value / 1.414214;
					}
					else
					{
						edlsession->ruler_x2 = edlsession->ruler_x1 + distance_value / 1.414214;
						edlsession->ruler_y2 = edlsession->ruler_y1 + distance_value / 1.414214;
					}
				}
				do_ruler(1, 0, 0, 0);
				get_canvas()->flash();
				gui->update_tool();
				break;
			}
		}
		else
		{
			if(canvas_cursor_x >= canvas_x1 - RULERHANDLE_W / 2 &&
					canvas_cursor_x < canvas_x1 + RULERHANDLE_W / 2 &&
					canvas_cursor_y >= canvas_y1 - RULERHANDLE_W &&
					canvas_cursor_y < canvas_y1 + RULERHANDLE_H / 2)
				set_cursor(UPRIGHT_ARROW_CURSOR);
			else if(canvas_cursor_x >= canvas_x2 - RULERHANDLE_W / 2 &&
					canvas_cursor_x < canvas_x2 + RULERHANDLE_W / 2 &&
					canvas_cursor_y >= canvas_y2 - RULERHANDLE_W &&
					canvas_cursor_y < canvas_y2 + RULERHANDLE_H / 2)
				set_cursor(UPRIGHT_ARROW_CURSOR);
			else
				set_cursor(CROSS_CURSOR);
// Update current position
			gui->update_tool();
		}
	}
// Assume no ruler measurement if 0 length
	if(draw && (!EQUIV(x2, x1) || !EQUIV(y2, y1)))
	{
		int icx1 = round(canvas_x1);
		int icy1 = round(canvas_y1);
		int icx2 = round(canvas_x2);
		int icy2 = round(canvas_y2);

		get_canvas()->set_inverse();
		get_canvas()->set_color(WHITE);
		get_canvas()->draw_line(icx1, icy1, icx2, icy2);
		get_canvas()->draw_line(icx1 - RULERHANDLE_W / 2,
			icy1, icx1 + RULERHANDLE_W / 2, icy1);
		get_canvas()->draw_line(icx1, icy1 - RULERHANDLE_H / 2,
			icx1, icy1 + RULERHANDLE_H / 2);
		get_canvas()->draw_line(icx2 - RULERHANDLE_W / 2, icy2,
			icx2 + RULERHANDLE_W / 2, icy2);
		get_canvas()->draw_line(icx2, icy2 - RULERHANDLE_H / 2,
			icx2, icy2 + RULERHANDLE_H / 2);
		get_canvas()->set_opaque();
	}
	return result;
}

int CWindowCanvas::do_mask(int &redraw, int &rerender, int button_press,
	int cursor_motion, int draw)
{
// Retrieve points from top recordable track
	MaskAutos *mask_autos;
	Track *track = gui->cwindow->calculate_affected_track();
	double projector_x, projector_y, projector_z;

	if(!track)
		return 0;

	mask_autos = (MaskAutos*)track->automation->have_autos(AUTOMATION_MASK);

	if(!mask_autos)
	{
		if(button_press)
			mask_autos = (MaskAutos*)track->automation->get_autos(AUTOMATION_MASK);
		else
			return 0;
	}

	ptstime position = master_edl->local_session->get_selectionstart(1);

	ArrayList<MaskPoint*> points;

	mask_autos->get_points(&points, edlsession->cwindow_mask, position);

	double track_w = track->track_w;
	double track_h = track->track_h;

// Projector zooms relative to the center of the track output.
	double half_track_w = track_w / 2;
	double half_track_h = track_h / 2;
// Translate mask to projection
	track->automation->get_projector(&projector_x,
		&projector_y,
		&projector_z,
		position);

// Get position of cursor relative to mask
	double mask_cursor_x = get_cursor_x();
	double mask_cursor_y = get_cursor_y();
	canvas_to_output(mask_cursor_x, mask_cursor_y);

	projector_x += edlsession->output_w / 2;
	projector_y += edlsession->output_h / 2;

	mask_cursor_x -= projector_x;
	mask_cursor_y -= projector_y;
	mask_cursor_x = mask_cursor_x / projector_z + half_track_w;
	mask_cursor_y = mask_cursor_y / projector_z + half_track_h;

// Fix cursor origin
	if(button_press)
	{
		gui->x_origin = mask_cursor_x;
		gui->y_origin = mask_cursor_y;
	}

	int result = 0;
// Points of closest line
	int shortest_point1 = -1;
	int shortest_point2 = -1;
// Closest point
	int shortest_point = -1;
// Distance to closest line
	double shortest_line_distance = BC_INFINITY;
// Distance to closest point
	double shortest_point_distance = BC_INFINITY;
	int selected_point = -1;
	int selected_control_point = -1;
	double selected_control_point_distance = BC_INFINITY;
	ArrayList<int> x_points;
	ArrayList<int> y_points;

	if(!cursor_motion)
	{
		if(draw)
		{
			get_canvas()->set_color(WHITE);
			get_canvas()->set_inverse();
		}

// Never draw closed polygon and a closed
// polygon is harder to add points to.
		for(int i = 0; i < points.total && !result; i++)
		{
			MaskPoint *point1 = points.values[i];
			MaskPoint *point2 = (i >= points.total - 1) ?
				points.values[0] :
				points.values[i + 1];
			double x0, x1, x2, x3;
			double y0, y1, y2, y3;
			double old_x, old_y, x, y;
			double p1x, p2x, p1y, p2y;
			double p1cx1, p1cx2, p2cx1, p2cx2;
			double p1cy1, p1cy2, p2cy1, p2cy2;
// Calculate frame coordinates
			p1x = point1->submask_x * track_w;
			p2x = point2->submask_x * track_w;
			p1y = point1->submask_y * track_h;
			p2y = point2->submask_y * track_h;
			p1cx1 = point1->control_x1 * track_w;
			p1cx2 = point1->control_x2 * track_w;
			p2cx1 = point2->control_x1 * track_w;
			p2cx2 = point2->control_x2 * track_w;
			p1cy1 = point1->control_y1 * track_h;
			p1cy2 = point1->control_y2 * track_h;
			p2cy1 = point2->control_y1 * track_h;
			p2cy2 = point2->control_y2 * track_h;

			int segments = round(sqrt(SQR(p1x - p2x) + SQR(p1y - p2y)));

			for(int j = 0; j <= segments && !result; j++)
			{
				if(segments)
				{
					x0 = p1x;
					y0 = p1y;
					x1 = p1x + p1cx2;
					y1 = p1y + p1cy2;
					x2 = p2x + p2cx1;
					y2 = p2y + p2cy1;
					x3 = p2x;
					y3 = p2y;

					double t = (double)j / segments;
					double tpow2 = t * t;
					double tpow3 = t * t * t;
					double invt = 1 - t;
					double invtpow2 = invt * invt;
					double invtpow3 = invt * invt * invt;

					x = (        invtpow3 * x0
						+ 3 * t     * invtpow2 * x1
						+ 3 * tpow2 * invt     * x2 
						+     tpow3            * x3);
					y = (        invtpow3 * y0 
						+ 3 * t     * invtpow2 * y1
						+ 3 * tpow2 * invt     * y2 
						+     tpow3            * y3);
				}
				else
				{
					x = x1 = x2 = p1x;
					y = y1 = y2 = p1y;
				}
				x = (x - half_track_w) *
					projector_z + projector_x;
				y = (y - half_track_h) *
					projector_z + projector_y;

// Test new point addition
				if(button_press)
				{
					double line_distance =
						sqrt(SQR(x - mask_cursor_x) + SQR(y - mask_cursor_y));

					if(line_distance < shortest_line_distance || 
						shortest_point1 < 0)
					{
						shortest_line_distance = line_distance;
						shortest_point1 = i;
						shortest_point2 =
							(i >= points.total - 1) ?
								0 : (i + 1);
					}

					double point_distance1 =
						sqrt(SQR(p1x - mask_cursor_x) +
							SQR(p1y - mask_cursor_y));
					double point_distance2 =
						sqrt(SQR(p2x - mask_cursor_x) +
							SQR(p2y - mask_cursor_y));

					if(point_distance1 < shortest_point_distance ||
						shortest_point < 0)
					{
						shortest_point_distance = point_distance1;
						shortest_point = i;
					}

					if(point_distance2 < shortest_point_distance ||
						shortest_point < 0)
					{
						shortest_point_distance = point_distance2;
						shortest_point = (i >= points.total - 1) ? 0 : (i + 1);
					}
				}

				output_to_canvas(x, y);

#define TEST_BOX(cursor_x, cursor_y, target_x, target_y) \
	(cursor_x >= target_x - CONTROL_W / 2 && \
	cursor_x < target_x + CONTROL_W / 2 && \
	cursor_y >= target_y - CONTROL_H / 2 && \
	cursor_y < target_y + CONTROL_H / 2)

// Test existing point selection
				if(button_press)
				{
					double canvas_x = (x0 - half_track_w) *
						projector_z + projector_x;
					double canvas_y = (y0 - half_track_h) *
						projector_z + projector_y;
					int cursor_x = get_cursor_x();
					int cursor_y = get_cursor_y();
// Test first point
					if(gui->shift_down())
					{
						double control_x = (x1 - half_track_w) *
							projector_z + projector_x;
						double control_y = (y1 - half_track_h) *
							projector_z + projector_y;
						output_to_canvas(control_x, control_y);

						double distance =
							sqrt(SQR(control_x - cursor_x) +
							SQR(control_y - cursor_y));

						if(distance < selected_control_point_distance)
						{
							selected_point = i;
							selected_control_point = 1;
							selected_control_point_distance = distance;
						}
					}
					else
					{
						output_to_canvas(canvas_x, canvas_y);
						if(!gui->ctrl_down())
						{
							if(TEST_BOX(cursor_x, cursor_y, canvas_x, canvas_y))
							{
								selected_point = i;
							}
						}
						else
						{
							selected_point = shortest_point;
						}
					}

// Test second point
					canvas_x = (x3 - half_track_w) *
						projector_z + projector_x;
					canvas_y = (y3 - half_track_h) *
						projector_z + projector_y;

					if(gui->shift_down())
					{
						double control_x = (x2 - half_track_w) * projector_z + projector_x;
						double control_y = (y2 - half_track_h) * projector_z + projector_y;
						output_to_canvas(control_x, control_y);

						double distance =
							sqrt(SQR(control_x - cursor_x) + SQR(control_y - cursor_y));

						if(distance < selected_control_point_distance)
						{
							selected_point = (i < points.total - 1 ? i + 1 : 0);
							selected_control_point = 0;
							selected_control_point_distance = distance;
						}
					}
					else if(i < points.total - 1)
					{
						output_to_canvas(canvas_x, canvas_y);

						if(!gui->ctrl_down())
						{
							if(TEST_BOX(cursor_x, cursor_y,
									canvas_x, canvas_y))
								selected_point =
									(i < points.total - 1 ? i + 1 : 0);
						}
						else
							selected_point = shortest_point;
					}
				}

				if(j > 0)
				{
// Draw joining line
					if(draw)
					{
						x_points.append((int)round(x));
						y_points.append((int)round(y));
					}

					if(j == segments)
					{
						if(draw)
						{
// Draw second anchor
							int ix = round(x);
							int iy = round(y);

							if(i < points.total - 1)
							{
								if(i == gui->affected_point - 1)
									get_canvas()->draw_disc(ix - CONTROL_W / 2,
										iy - CONTROL_W / 2,
										CONTROL_W, 
										CONTROL_W);
								else
									get_canvas()->draw_circle(ix - CONTROL_W / 2,
										iy - CONTROL_W / 2,
										CONTROL_W, 
										CONTROL_W);
							}

// Draw second control point.  Discard x2 and y2 after this.
							x2 = (x2 - half_track_w) *
								projector_z + projector_x;
							y2 = (y2 - half_track_h) *
								projector_z + projector_y;
							output_to_canvas(x2, y2);
							int ix2 = round(x2);
							int iy2 = round(y2);
							get_canvas()->draw_line(ix, iy, ix2, iy2);
							get_canvas()->draw_rectangle(ix2 - CONTROL_W / 2,
								iy2 - CONTROL_H / 2,
								CONTROL_W,
								CONTROL_H);
						}
					}
				}
				else
				{
// Draw first anchor
					if(i == 0 && draw)
					{
						get_canvas()->draw_disc((int)round(x) - FIRST_CONTROL_W / 2,
							(int)round(y) - FIRST_CONTROL_H / 2,
							FIRST_CONTROL_W,
							FIRST_CONTROL_H);
					}

// Draw first control point.  Discard x1 and y1 after this.
					if(draw)
					{
						x1 = (x1 - half_track_w) * projector_z + projector_x;
						y1 = (y1 - half_track_h) * projector_z + projector_y;
						output_to_canvas(x1, y1);
						int ix = round(x);
						int iy = round(y);
						int ix1 = round(x1);
						int iy1 = round(y1);
						get_canvas()->draw_line(ix, iy, ix1, iy1);
						get_canvas()->draw_rectangle(ix1 - CONTROL_W / 2,
							iy1 - CONTROL_H / 2,
							CONTROL_W,
							CONTROL_H);
						x_points.append(ix);
						y_points.append(iy);
					}
				}

				old_x = x;
				old_y = y;
			}
		}

		if(draw)
		{
			get_canvas()->draw_polygon(&x_points, &y_points);
			get_canvas()->set_opaque();
		}
	}

	if(button_press && !result)
	{
		gui->affected_track = gui->cwindow->calculate_affected_track();
// Get current keyframe
		if(gui->affected_track)
			gui->affected_keyframe =
				gui->cwindow->calculate_affected_auto(AUTOMATION_MASK,
					position, gui->affected_track, 1);

		MaskAuto *keyframe = (MaskAuto*)gui->affected_keyframe;
		SubMask *mask = keyframe->create_submask(edlsession->cwindow_mask);

// Translate entire keyframe
		if(gui->alt_down() && mask->points.total)
		{
			gui->current_operation = CWINDOW_MASK_TRANSLATE;
			gui->affected_point = 0;
		}
		else
// Existing point or control point was selected
		if(selected_point >= 0)
		{
			gui->affected_point = selected_point;

			if(selected_control_point == 0)
				gui->current_operation = CWINDOW_MASK_CONTROL_IN;
			else
			if(selected_control_point == 1)
				gui->current_operation = CWINDOW_MASK_CONTROL_OUT;
			else
				gui->current_operation = edlsession->cwindow_operation;
		}
		else
// No existing point or control point was selected so create a new one
		if(!gui->shift_down() && !gui->alt_down())
		{
// Create the template
			MaskPoint *point = new MaskPoint;

			point->submask_x = mask_cursor_x / track_w;
			point->submask_y = mask_cursor_y / track_h;

			if(shortest_point2 < shortest_point1)
			{
				shortest_point2 ^= shortest_point1;
				shortest_point1 ^= shortest_point2;
				shortest_point2 ^= shortest_point1;
			}
// Append to end of list
			if(abs(shortest_point1 - shortest_point2) > 1 || mask->points.total == 1)
			{
// Need to apply the new point to every keyframe
				for(MaskAuto *current = (MaskAuto*)mask_autos->first;
					current; current = (MaskAuto*)NEXT)
				{
					SubMask *submask = current->create_submask(edlsession->cwindow_mask);
					submask->append_point(point);
				}

				gui->affected_point = mask->points.total - 1;
				result = 1;
			}
			else
// Insert between 2 points, shifting back point 2
			if(shortest_point1 >= 0 && shortest_point2 >= 0)
			{
				for(MaskAuto *current = (MaskAuto*)mask_autos->first;
					current; current = (MaskAuto*)NEXT)
				{
					SubMask *submask = current->create_submask(edlsession->cwindow_mask);
// In case the keyframe point count isn't synchronized with the rest of the keyframes,
// avoid a crash.
					if(submask->points.total >= shortest_point2)
					{
						submask->points.append(0);
						for(int i = submask->points.total - 1;
								i > shortest_point2; i--)
							submask->points.values[i] = submask->points.values[i - 1];
						MaskPoint *new_point = new MaskPoint;
						submask->points.values[shortest_point2] = new_point;
						*new_point = *point;
					}
				}

				gui->affected_point = shortest_point2;
				result = 1;
			}

// Create the first point.
			if(!result)
			{
				for(MaskAuto *current = (MaskAuto*)mask_autos->first;
					current; current = (MaskAuto*)NEXT)
				{
					SubMask *submask = current->create_submask(edlsession->cwindow_mask);
					submask->append_point(point);
				}
				gui->affected_point = mask->points.total - 1;
			}

			gui->current_operation = edlsession->cwindow_operation;
// Delete the template
			delete point;
			mwindow_global->undo->update_undo(_("mask point"), LOAD_AUTOMATION);
		}

		result = 1;
		rerender = 1;
		redraw = 1;
	}

	if(button_press && result)
	{
		MaskAuto *keyframe = (MaskAuto*)gui->affected_keyframe;
		SubMask *mask = keyframe->get_submask(edlsession->cwindow_mask);
		MaskPoint *point = mask->points.values[gui->affected_point];

		gui->center_x = point->submask_x * track_w;
		gui->center_y = point->submask_y * track_h;
		gui->control_in_x = point->control_x1 * track_w;
		gui->control_in_y = point->control_y1 * track_h;
		gui->control_out_x = point->control_x2 * track_w;
		gui->control_out_y = point->control_y2 * track_h;
	}

	if(cursor_motion)
	{
		MaskAuto *keyframe = (MaskAuto*)gui->affected_keyframe;
		SubMask *mask = keyframe->get_submask(edlsession->cwindow_mask);

		if(gui->affected_point < mask->points.total)
		{
			MaskPoint *point = mask->points.values[gui->affected_point];
			double cursor_x = mask_cursor_x;
			double cursor_y = mask_cursor_y;

			double last_x = point->submask_x;
			double last_y = point->submask_y;
			double last_control_x1 = point->control_x1;
			double last_control_y1 = point->control_y1;
			double last_control_x2 = point->control_x2;
			double last_control_y2 = point->control_y2;

			switch(gui->current_operation)
			{
			case CWINDOW_MASK:
				point->submask_x = (cursor_x - gui->x_origin +
					gui->center_x) / track_w;
				point->submask_y = (cursor_y - gui->y_origin +
					gui->center_y) / track_h;
				break;

			case CWINDOW_MASK_CONTROL_IN:
				point->control_x1 = (cursor_x - gui->x_origin +
					gui->control_in_x) / track_w;
				point->control_y1 = (cursor_y - gui->y_origin +
					gui->control_in_y) / track_h;
				break;

			case CWINDOW_MASK_CONTROL_OUT:
				point->control_x2 = (cursor_x - gui->x_origin +
					gui->control_out_x) / track_w;
				point->control_y2 = (cursor_y - gui->y_origin +
					gui->control_out_y) / track_h;
				break;

			case CWINDOW_MASK_TRANSLATE:
				for(int i = 0; i < mask->points.total; i++)
				{
					mask->points.values[i]->submask_x +=
						(cursor_x - gui->x_origin) / track_w;
					mask->points.values[i]->submask_y +=
						(cursor_y - gui->y_origin) / track_h;
				}
				gui->x_origin = cursor_x;
				gui->y_origin = cursor_y;
				break;
			}

			if(!EQUIV(last_x, point->submask_x) ||
				!EQUIV(last_y, point->submask_y) ||
				!EQUIV(last_control_x1, point->control_x1) ||
				!EQUIV(last_control_y1, point->control_y1) ||
				!EQUIV(last_control_x2, point->control_x2) ||
				!EQUIV(last_control_y2, point->control_y2))
			{
				rerender = 1;
				redraw = 1;
			}
		}
		result = 1;
	}

	points.remove_all_objects();
	return result;
}

int CWindowCanvas::do_eyedrop(int &rerender, int button_press)
{
	int result = 0;
	double cursor_x = get_cursor_x();
	double cursor_y = get_cursor_y();

	if(button_press)
		gui->current_operation = CWINDOW_EYEDROP;

	if(gui->current_operation == CWINDOW_EYEDROP)
	{
		canvas_to_output(cursor_x, cursor_y);

// Get color out of frame.
		if(refresh_frame)
		{
			int in_x = round(CLIP(cursor_x, 0, refresh_frame->get_w() - 1));
			int in_y = round(CLIP(cursor_y, 0, refresh_frame->get_h() - 1));
			uint16_t *pixel;

			switch(refresh_frame->get_color_model())
			{
			case BC_RGBA16161616:
				pixel =
					&((uint16_t*)refresh_frame->get_row_ptr(in_y))[in_x * 4];
				master_edl->local_session->set_picker_rgb(
					pixel[0], pixel[1], pixel[2]);
				break;
			case BC_AYUV16161616:
				pixel =
					&((uint16_t*)refresh_frame->get_row_ptr(in_y))[in_x * 4];
				master_edl->local_session->set_picker_yuv(
					pixel[1], pixel[2], pixel[3]);
				break;
			}
		}
		else
			master_edl->local_session->set_picker_rgb(0, 0, 0);

		gui->update_tool();
		result = 1;
	}
	return result;
}

void CWindowCanvas::draw_overlays()
{
	draw_camera();
	draw_crop();
	safe_regions->set_enabled(edlsession->safe_regions);
	guidelines.draw(master_edl,
		master_edl->local_session->get_selectionstart(1));

	if(edlsession->cwindow_scrollbars)
	{
// Always draw output rectangle
		double x1, y1, x2, y2;
		x1 = 0;
		x2 = edlsession->output_w;
		y1 = 0;
		y2 = edlsession->output_h;
		output_to_canvas(x1, y1);
		output_to_canvas(x2, y2);

		get_canvas()->set_inverse();
		get_canvas()->set_color(WHITE);

		get_canvas()->draw_rectangle(round(x1), round(y1),
			round(x2 - x1), round(y2 - y1));

		get_canvas()->set_opaque();
	}

	if(mainsession->ccanvas_highlighted)
	{
		get_canvas()->set_color(WHITE);
		get_canvas()->set_inverse();
		get_canvas()->draw_rectangle(0, 0, get_canvas()->get_w(),
			get_canvas()->get_h());
		get_canvas()->draw_rectangle(1, 1, get_canvas()->get_w() - 2,
			get_canvas()->get_h() - 2);
		get_canvas()->set_opaque();
	}

	int temp1 = 0, temp2 = 0;

	switch(edlsession->cwindow_operation)
	{
	case CWINDOW_MASK:
		do_mask(temp1, temp2, 0, 0, 1);
		break;

	case CWINDOW_RULER:
		do_ruler(1, 0, 0, 0);
		break;
	}
}

void CWindowCanvas::update_guidelines()
{
	safe_regions->clear();
	safe_regions->add_rectangle(round(0.05 * edlsession->output_w),
		round(0.05 * edlsession->output_h),
		round(0.9 * edlsession->output_w),
		round(0.9 * edlsession->output_h));
	safe_regions->add_rectangle(round(0.1 * edlsession->output_w),
			round(0.1 * edlsession->output_h),
		round(0.8 * edlsession->output_w),
		round(0.8 * edlsession->output_h));
}

void CWindowCanvas::reset_camera_auto(int is_camera)
{
	FloatAuto *x_keyframe;
	FloatAuto *y_keyframe;
	FloatAuto *z_keyframe;
	Track *affected_track;

	affected_track = gui->cwindow->calculate_affected_track();

	if(affected_track)
	{
		gui->cwindow->calculate_affected_autos(&x_keyframe,
			&y_keyframe, &z_keyframe, affected_track,
			is_camera, 1, 1, 1);

		x_keyframe->set_value(0);
		y_keyframe->set_value(0);
		z_keyframe->set_value(1);

		mwindow_global->sync_parameters();
		gui->update_tool();
	}
}

void CWindowCanvas::reset_camera()
{
	reset_camera_auto(1);
}

void CWindowCanvas::reset_projector()
{
	reset_camera_auto(0);
}

int CWindowCanvas::test_crop(int button_press, int *redraw, int *rerender)
{
	int result = 0;
	int do_redraw = *redraw;
	int handle_selected = -1;
	double x1, y1, x2, y2;
	double cursor_x = get_cursor_x();
	double cursor_y = get_cursor_y();
	double canvas_x1, canvas_y1, canvas_x2, canvas_y2;
	double canvas_cursor_x = cursor_x;
	double canvas_cursor_y = cursor_y;
	Track *track;
	int created_auto;
	double top, left, right, bottom;
	CropAuto *crop_auto;
	CropAutos *crop_autos;

	track = gui->cwindow->calculate_affected_track();

	if(!track)
		return 0;

	crop_autos = (CropAutos*)track->automation->have_autos(AUTOMATION_CROP);

	if(!crop_autos)
	{
		if(button_press)
			crop_autos = (CropAutos*)track->automation->get_autos(AUTOMATION_CROP);
		else
			return 0;
	}

	ptstime pts = master_edl->local_session->get_selectionstart(1);

	crop_autos->get_values(pts, &left, &right, &top, &bottom);

	crop_auto = (CropAuto*)gui->cwindow->calculate_affected_auto(AUTOMATION_CROP,
		pts, track, 1, &created_auto);
	if(created_auto)
	{
		crop_auto->left = left;
		crop_auto->right = right;
		crop_auto->top = top;
		crop_auto->bottom = bottom;
	}

	canvas_x1 = x1 = round(left * track->track_w);
	canvas_x2 = x2 = round(right * track->track_w);
	canvas_y1 = y1 = round(top * track->track_h);
	canvas_y2 = y2 = round(bottom * track->track_h);

	canvas_to_output(cursor_x, cursor_y);
// Use screen normalized coordinates for hot spot tests.
	output_to_canvas(canvas_x1, canvas_y1);
	output_to_canvas(canvas_x2, canvas_y2);

	if(gui->current_operation == CWINDOW_CROP)
	{
		handle_selected = gui->crop_handle;
	}
	else
	if(canvas_cursor_x >= canvas_x1 && canvas_cursor_x < canvas_x1 + CROPHANDLE_W &&
		canvas_cursor_y >= canvas_y1 && canvas_cursor_y < canvas_y1 + CROPHANDLE_H)
	{
		handle_selected = 0;
		gui->crop_origin_x = x1;
		gui->crop_origin_y = y1;
	}
	else
	if(canvas_cursor_x >= canvas_x2 - CROPHANDLE_W && canvas_cursor_x < canvas_x2 &&
		canvas_cursor_y >= canvas_y1 && canvas_cursor_y < canvas_y1 + CROPHANDLE_H)
	{
		handle_selected = 1;
		gui->crop_origin_x = x2;
		gui->crop_origin_y = y1;
	}
	else
	if(canvas_cursor_x >= canvas_x1 && canvas_cursor_x < canvas_x1 + CROPHANDLE_W &&
		canvas_cursor_y >= canvas_y2 - CROPHANDLE_H && canvas_cursor_y < canvas_y2)
	{
		handle_selected = 2;
		gui->crop_origin_x = x1;
		gui->crop_origin_y = y2;
	}
	else
	if(canvas_cursor_x >= canvas_x2 - CROPHANDLE_W && canvas_cursor_x < canvas_x2 &&
		canvas_cursor_y >= canvas_y2 - CROPHANDLE_H && canvas_cursor_y < canvas_y2)
	{
		handle_selected = 3;
		gui->crop_origin_x = x2;
		gui->crop_origin_y = y2;
	}

// Start dragging
	if(button_press)
	{
		if(gui->alt_down())
		{
			gui->crop_translate = 1;
			gui->crop_origin_x1 = x1;
			gui->crop_origin_y1 = y1;
			gui->crop_origin_x2 = x2;
			gui->crop_origin_y2 = y2;
		}
		else
			gui->crop_translate = 0;

		gui->current_operation = CWINDOW_CROP;
		gui->crop_handle = handle_selected;
		gui->x_origin = cursor_x;
		gui->y_origin = cursor_y;
		result = 1;

		if(handle_selected < 0 && !gui->crop_translate) 
			do_redraw = 1;
	}
	else
// Translate all 4 points
	if(gui->current_operation == CWINDOW_CROP && gui->crop_translate)
	{
		x1 = cursor_x - gui->x_origin + gui->crop_origin_x1;
		y1 = cursor_y - gui->y_origin + gui->crop_origin_y1;
		x2 = cursor_x - gui->x_origin + gui->crop_origin_x2;
		y2 = cursor_y - gui->y_origin + gui->crop_origin_y2;

		crop_auto->left = x1 / track->track_w;
		crop_auto->top = y1 / track->track_h;
		crop_auto->right = x2 / track->track_w;
		crop_auto->bottom = y2 / track->track_h;
		result = 1;
		do_redraw = 1;
	}
	else
// Update dragging
	if(gui->current_operation == CWINDOW_CROP)
	{
		switch(gui->crop_handle)
		{
		case -1:
			x1 = gui->crop_origin_x;
			y1 = gui->crop_origin_y;
			x2 = gui->crop_origin_x;
			y2 = gui->crop_origin_y;
			if(cursor_x < gui->x_origin)
			{
				if(cursor_y < gui->y_origin)
				{
					x1 = cursor_x;
					y1 = cursor_y;
				}
				else
				if(cursor_y >= gui->y_origin)
				{
					x1 = cursor_x;
					y2 = cursor_y;
				}
			}
			else
			if(cursor_x  >= gui->x_origin)
			{
				if(cursor_y < gui->y_origin)
				{
					y1 = cursor_y;
					x2 = cursor_x;
				}
				else
				if(cursor_y >= gui->y_origin)
				{
					x2 = cursor_x;
					y2 = cursor_y;
				}
			}
			break;
		case 0:
			x1 = cursor_x - gui->x_origin + gui->crop_origin_x;
			y1 = cursor_y - gui->y_origin + gui->crop_origin_y;
			break;
		case 1:
			x2 = cursor_x - gui->x_origin + gui->crop_origin_x;
			y1 = cursor_y - gui->y_origin + gui->crop_origin_y;
			break;
		case 2:
			x1 = cursor_x - gui->x_origin + gui->crop_origin_x;
			y2 = cursor_y - gui->y_origin + gui->crop_origin_y;
			break;
		case 3:
			x2 = cursor_x - gui->x_origin + gui->crop_origin_x;
			y2 = cursor_y - gui->y_origin + gui->crop_origin_y;
			break;
		}

		double cl, cr, ct, cb;

		cl = round(crop_auto->left * track->track_w);
		cr = round(crop_auto->right * track->track_w);
		ct = round(crop_auto->top * track->track_h);
		cb = round(crop_auto->bottom * track->track_h);

		if(!EQUIV(cl, x1) || !EQUIV(cr, x2) ||
			!EQUIV(ct, y1) || !EQUIV(cb, y2))
		{
			if(x1 > x2)
			{
				double tmp = x1;
				x1 = x2;
				x2 = tmp;
				switch(gui->crop_handle)
				{
				case 0:
					gui->crop_handle = 1;
					break;
				case 1:
					gui->crop_handle = 0;
					break;
				case 2:
					gui->crop_handle = 3;
					break;
				case 3:
					gui->crop_handle = 2;
					break;
				default:
					break;
				}
			}
			if(y1 > y2)
			{
				double tmp = y1;
				y1 = y2;
				y2 = tmp;
				switch(gui->crop_handle)
				{
				case 0:
					gui->crop_handle = 2;
					break;
				case 1:
					gui->crop_handle = 3;
					break;
				case 2:
					gui->crop_handle = 0;
					break;
				case 3:
					gui->crop_handle = 1;
					break;
				default:
					break;
				}
			}

			crop_auto->left = x1 / track->track_w;
			crop_auto->top = y1 / track->track_h;
			crop_auto->right = x2 / track->track_w;
			crop_auto->bottom = y2 / track->track_h;
			result = 1;
			do_redraw = 1;
		}
	}
	else
// Update cursor font
	if(handle_selected >= 0)
	{
		switch(handle_selected)
		{
		case 0:
			set_cursor(UPLEFT_RESIZE);
			break;
		case 1:
			set_cursor(UPRIGHT_RESIZE);
			break;
		case 2:
			set_cursor(DOWNLEFT_RESIZE);
			break;
		case 3:
			set_cursor(DOWNRIGHT_RESIZE);
			break;
		}
		result = 1;
	}
	else
		set_cursor(ARROW_CURSOR);

	if(do_redraw)
	{
		CLAMP(crop_auto->left, 0, 1.0);
		CLAMP(crop_auto->right, 0, 1.0);
		CLAMP(crop_auto->top, 0, 1.0);
		CLAMP(crop_auto->bottom, 0, 1.0);
		*redraw = do_redraw;
		if(rerender)
			*rerender = 1;
	}
	return result;
}

void CWindowCanvas::draw_crop()
{
	Track *track = gui->cwindow->calculate_affected_track();
	CropAutos *crop_autos;
	ptstime position;
	int left, right, top, bottom;

	if(!track)
		return;

	for(Track *current = track->tracks->first; current; current = current->next)
		if(current->crop_gframe)
			current->crop_gframe->set_enabled(0);

	crop_autos = (CropAutos*)track->automation->have_autos(AUTOMATION_CROP);

	if(!crop_autos)
		return;

	position = master_edl->local_session->get_selectionstart(1);

	crop_autos->get_values(position, &left, &right, &top, &bottom);

	if(!track->crop_gframe)
		mwindow_global->new_guideframe(0, MAX_PTSTIME, &track->crop_gframe);

	track->crop_gframe->clear();
	track->crop_gframe->add_rectangle(left, top, right - left, bottom - top);
	track->crop_gframe->add_box(left, top, CROPHANDLE_W, CROPHANDLE_H);
	track->crop_gframe->add_box(right - CROPHANDLE_W, top, CROPHANDLE_W, CROPHANDLE_H);
	track->crop_gframe->add_box(left, bottom - CROPHANDLE_H, CROPHANDLE_W, CROPHANDLE_H);
	track->crop_gframe->add_box(right - CROPHANDLE_W, bottom - CROPHANDLE_H, CROPHANDLE_W, CROPHANDLE_H);
	track->crop_gframe->set_enabled(1);
}

void CWindowCanvas::draw_camera()
{
	Track *track = gui->cwindow->calculate_affected_track();
	double center_x, center_y, center_z;
	GuideFrame **gframep;
	int is_camera;
	int track_x1, track_y1, track_x2, track_y2;
	double h_trackw, h_trackh;

	if(!track)
		return;

	for(Track *current = track->tracks->first; current; current = current->next)
	{
		if(current->camera_gframe)
			current->camera_gframe->set_enabled(0);
		if(current->projector_gframe)
			current->projector_gframe->set_enabled(0);
	}

	switch(edlsession->cwindow_operation)
	{
	case CWINDOW_CAMERA:
		is_camera = 1;
		break;
	case CWINDOW_PROJECTOR:
		is_camera = 0;
		break;
	default:
		return;
	}

	ptstime position = master_edl->local_session->get_selectionstart(1);

	h_trackw = track->track_w / 2;
	h_trackh = track->track_h / 2;

	if(is_camera)
	{
		track_x1 = track_y1 = 0;
		track_x2 = track->track_w;
		track_y2 = track->track_h;
		gframep = &track->camera_gframe;
	}
	else
	{
		track->automation->get_projector(&center_x, &center_y, &center_z,
			position);
		center_x += edlsession->output_w / 2.0;
		center_y += edlsession->output_h / 2.0;
		track_x1 = round(center_x - h_trackw * center_z);
		track_y1 = round(center_y - h_trackh * center_z);
		track_x2 = round(track_x1 + track->track_w * center_z);
		track_y2 = round(track_y1 + track->track_h * center_z);
		gframep = &track->projector_gframe;
	}

	if(!*gframep)
	{
		mwindow_global->new_guideframe(0, MAX_PTSTIME, gframep);
		(*gframep)->set_color(is_camera ? GREEN : RED);
		(*gframep)->set_opaque(1);
	}

	(*gframep)->clear();
	(*gframep)->add_rectangle(track_x1, track_y1, track_x2 - track_x1,
		track_y2 - track_y1);
	(*gframep)->add_line(track_x1, track_y1, track_x2, track_y2);
	(*gframep)->add_line(track_x2, track_y1, track_x1, track_y2);
	(*gframep)->set_enabled(1);
}

int CWindowCanvas::do_camera(int button_press, int &redraw, int &redraw_canvas,
	int &rerender)
{
	int result = 0;

// Processing drag operation.
// Create keyframe during first cursor motion.
	if(!button_press)
	{
		if(gui->current_operation == CWINDOW_CAMERA ||
			gui->current_operation == CWINDOW_PROJECTOR)
		{
			if(!gui->ctrl_down() && gui->shift_down() && !gui->translating_zoom)
			{
				gui->translating_zoom = 1;
				gui->reset_affected();
			}
			else
			if(!gui->ctrl_down() && !gui->shift_down() && gui->translating_zoom)
			{
				gui->translating_zoom = 0;
				gui->reset_affected();
			}

// Get target keyframe
			double last_center_x;
			double last_center_y;
			double last_center_z;
			int created;

			if(!gui->affected_x && !gui->affected_y && !gui->affected_z)
			{
				int x_auto_ix, y_auto_ix, z_auto_ix;
				ptstime pts = master_edl->local_session->get_selectionstart(1);

				if(!gui->affected_track) return 0;

				if(edlsession->cwindow_operation == CWINDOW_CAMERA)
				{
					x_auto_ix = AUTOMATION_CAMERA_X;
					y_auto_ix = AUTOMATION_CAMERA_Y;
					z_auto_ix = AUTOMATION_CAMERA_Z;
				}
				else
				{
					x_auto_ix = AUTOMATION_PROJECTOR_X;
					y_auto_ix = AUTOMATION_PROJECTOR_Y;
					z_auto_ix = AUTOMATION_PROJECTOR_Z;
				}

				if(gui->translating_zoom)
				{
					gui->affected_z = 
						(FloatAuto*)gui->cwindow->calculate_affected_auto(
							z_auto_ix, pts, gui->affected_track,
							1, &created, 0);
					if(created)
						redraw_canvas = 1;
				}
				else
				{
					gui->affected_x = 
						(FloatAuto*)gui->cwindow->calculate_affected_auto(
							x_auto_ix, pts, gui->affected_track,
							1, &created, 0);
					if(created)
						redraw_canvas = 1;
					gui->affected_y =
						(FloatAuto*)gui->cwindow->calculate_affected_auto(
							y_auto_ix, pts, gui->affected_track,
							1, &created, 0);
					if(created)
						redraw_canvas = 1;
				}

				calculate_origin();

				if(gui->translating_zoom)
					gui->center_z = gui->affected_z->get_value();
				else
				{
					gui->center_x = gui->affected_x->get_value();
					gui->center_y = gui->affected_y->get_value();
				}

				rerender = 1;
				redraw = 1;
			}

			if(gui->translating_zoom)
				last_center_z = gui->affected_z->get_value();
			else
			{
				last_center_x = gui->affected_x->get_value();
				last_center_y = gui->affected_y->get_value();
			}

			double cursor_x = get_cursor_x();
			double cursor_y = get_cursor_y();
			canvas_to_output(cursor_x, cursor_y);

			if(gui->translating_zoom)
			{
				double new_z = gui->center_z +
					(cursor_y - gui->y_origin) / 128;

				if(new_z < 0)
					new_z = 0;

				if(!EQUIV(last_center_z, new_z))
				{
					gui->affected_z->set_value(new_z);
					rerender = 1;
					redraw = 1;
					redraw_canvas = 1;
				}
			}
			else
			{

				double new_x = gui->center_x + cursor_x - gui->x_origin;
				double new_y = gui->center_y + cursor_y - gui->y_origin;

				if(!EQUIV(last_center_x,  new_x) ||
					!EQUIV(last_center_y, new_y))
				{
					gui->affected_x->set_value(new_x);
					gui->affected_y->set_value(new_y);
					rerender = 1;
					redraw = 1;
					redraw_canvas = 1;
				}
			}
		}

		result = 1;
	}
	else
// Begin drag operation.  Don't create keyframe here.
	{
// Get affected track off of the first recordable video track.
// Calculating based on the alpha channel would require recording what layer
// each output pixel belongs to as they're rendered and stacked.  Forget it.
		gui->affected_track = gui->cwindow->calculate_affected_track();
		gui->reset_affected();

		if(gui->affected_track)
		{
			gui->current_operation = 
				edlsession->cwindow_operation;
			result = 1;
		}
	}

	return result;
}

void CWindowCanvas::test_zoom(int &redraw)
{
	double zoom = get_zoom();
	int x, y, w, h, max;
	int fw, fh;
	int current_index;
	int mousebutton = get_buttonpress();
	BC_WindowBase *cur_canvas = get_canvas();

// Filter mouse buttons
	switch(mousebutton)
	{
	case 1:
	case 4:
	case 5:
		break;
	default:
		return;
	}

	cur_canvas->get_dimensions(&w, &h);

	if(!edlsession->cwindow_scrollbars)
	{
		edlsession->cwindow_scrollbars = 1;
		zoom = 1.0;
		x = edlsession->output_w / 2;
		y = edlsession->output_h / 2;
	}
	else
	{
		cur_canvas->get_relative_cursor_pos(&x, &y);
// Find current zoom in table
		for(current_index = 0 ; current_index < total_zooms; current_index++)
			if(EQUIV(my_zoom_table[current_index], zoom))
				break;

		if(mousebutton == 5)        // Zoom out
			current_index--;
		else if(mousebutton == 4)   // Zoom in
			current_index++;

		CLAMP(current_index, 0, total_zooms - 1);
		zoom = my_zoom_table[current_index];
	}

	fw = edlsession->output_w;
	fh = edlsession->output_h;

	x = round((double)x * fw / w) - w / 2;
	y = round((double)y * fh / h) - h / 2;

	w = round(w / zoom / sample_aspect_ratio());
	h = round(h / zoom);

	max = fw - w;
	if(max > 0 && x > max)
		x = max;
	if(x < 0 || max < 0)
		x = 0;
	max = fh - h;
	if(max > 0 && y > max)
		y = max;
	if(y < 0 || max < 0)
		y = 0;

	update_zoom(x, y, zoom);
	reposition_window(master_edl, theme_global->ccanvas_x,
		theme_global->ccanvas_y, theme_global->ccanvas_w,
		theme_global->ccanvas_h);
	update_scrollbars();

	clear_canvas();
	redraw = 1;

	gui->zoom_panel->update(zoom);
}

void CWindowCanvas::calculate_origin()
{
	gui->x_origin = get_cursor_x();
	gui->y_origin = get_cursor_y();
	canvas_to_output(gui->x_origin, gui->y_origin);
}

int CWindowCanvas::cursor_leave_event()
{
	set_cursor(ARROW_CURSOR);
	return 1;
}

int CWindowCanvas::cursor_enter_event()
{
	int redraw = 0;
	switch(edlsession->cwindow_operation)
	{
	case CWINDOW_CAMERA:
	case CWINDOW_PROJECTOR:
		set_cursor(MOVE_CURSOR);
		break;
	case CWINDOW_ZOOM:
		set_cursor(MOVE_CURSOR);
		break;
	case CWINDOW_CROP:
		test_crop(0, &redraw);
		break;
	case CWINDOW_PROTECT:
		set_cursor(ARROW_CURSOR);
		break;
	case CWINDOW_MASK:
	case CWINDOW_RULER:
		set_cursor(CROSS_CURSOR);
		break;
	case CWINDOW_EYEDROP:
		set_cursor(CROSS_CURSOR);
		break;
	}
	return 1;
}

int CWindowCanvas::cursor_motion_event()
{
	int redraw = 0, result = 0, rerender = 0, redraw_canvas = 0;
	int x, y;
	double zoom, cursor_x, cursor_y;
	double zoom_x, zoom_y, conformed_w, conformed_h;

	switch(gui->current_operation)
	{
	case CWINDOW_SCROLL:
		zoom = get_zoom();
		cursor_x = get_cursor_x();
		cursor_y = get_cursor_y();

		get_zooms(zoom_x, zoom_y, conformed_w, conformed_h);
		cursor_x = cursor_x / zoom_x + gui->x_offset;
		cursor_y = cursor_y / zoom_y + gui->y_offset;

		x = round(gui->x_origin - cursor_x + gui->x_offset);
		y = round(gui->y_origin - cursor_y + gui->y_offset);

		update_zoom(x, y, zoom);
		update_scrollbars();
		redraw = 1;
		result = 1;
		break;

	case CWINDOW_RULER:
		result = do_ruler(0, 1, 0, 0);
		break;

	case CWINDOW_CAMERA:
	case CWINDOW_PROJECTOR:
		result = do_camera(0, redraw, redraw_canvas, rerender);
		break;

	case CWINDOW_CROP:
		result = test_crop(0, &redraw, &rerender);
		break;

	case CWINDOW_MASK:
	case CWINDOW_MASK_CONTROL_IN:
	case CWINDOW_MASK_CONTROL_OUT:
	case CWINDOW_MASK_TRANSLATE:
		result = do_mask(redraw, rerender, 0, 1, 0);
		break;

	case CWINDOW_EYEDROP:
		result = do_eyedrop(rerender, 0);
		break;
	}

	if(!result)
	{
		switch(edlsession->cwindow_operation)
		{
		case CWINDOW_CROP:
			result = test_crop(0, &redraw, &rerender);
			break;

		case CWINDOW_RULER:
			result = do_ruler(0, 1, 0, 0);
			break;
		}
	}

// If the window is never unlocked before calling send_command the
// display shouldn't get stuck on the old video frame although it will
// flicker between the old video frame and the new video frame.
	if(redraw)
	{
		draw_refresh();
		gui->update_tool();
	}

	if(redraw_canvas)
		mwindow_global->draw_canvas_overlays();

	if(rerender)
	{
		mwindow_global->sync_parameters();
		if(!redraw) gui->update_tool();
	}
	return result;
}

int CWindowCanvas::button_press_event()
{
	int result = 0;
	int redraw = 0;
	int redraw_canvas = 0;
	int rerender = 0;
	double zoom_x, zoom_y, conformed_w, conformed_h;

	if(Canvas::button_press_event())
		return 1;

	gui->translating_zoom = gui->shift_down(); 

	calculate_origin();

	get_zooms(zoom_x, zoom_y, conformed_w, conformed_h);
	gui->x_offset = get_x_offset(zoom_x, conformed_w, conformed_h);
	gui->y_offset = get_y_offset(zoom_y, conformed_w, conformed_h);

// Scroll view
	if(get_buttonpress() == 2)
	{
		gui->current_operation = CWINDOW_SCROLL;
		result = 1;
	}
	else
// Adjust parameter
	{
		switch(edlsession->cwindow_operation)
		{
		case CWINDOW_RULER:
			result = do_ruler(0, 0, 1, 0);
			break;

		case CWINDOW_CAMERA:
		case CWINDOW_PROJECTOR:
			result = do_camera(1, redraw, redraw_canvas, rerender);
			break;

		case CWINDOW_ZOOM:
			test_zoom(redraw);
			result = 1;
			break;

		case CWINDOW_CROP:
			result = test_crop(1, &redraw, &rerender);
			break;

		case CWINDOW_MASK:
			if(get_buttonpress() == 1)
				result = do_mask(redraw, rerender, 1, 0, 0);
			break;

		case CWINDOW_EYEDROP:
			result = do_eyedrop(rerender, 1);
			break;
		}
	}

	if(redraw)
	{
		draw_refresh();
		gui->update_tool();
	}

// rerendering can also be caused by press event
	if(rerender)
	{
		mwindow_global->restart_brender();
		gui->cwindow->playback_engine->send_command(CURRENT_FRAME);
		if(!redraw) gui->update_tool();
	}
	return result;
}

int CWindowCanvas::button_release_event()
{
	int result = 0;

	switch(gui->current_operation)
	{
	case CWINDOW_SCROLL:
		result = 1;
		break;

	case CWINDOW_RULER:
		do_ruler(0, 0, 0, 1);
		break;

	case CWINDOW_CAMERA:
		mwindow_global->undo->update_undo(_("camera"), LOAD_AUTOMATION);
		break;

	case CWINDOW_PROJECTOR:
		mwindow_global->undo->update_undo(_("projector"), LOAD_AUTOMATION);
		break;

	case CWINDOW_MASK:
	case CWINDOW_MASK_CONTROL_IN:
	case CWINDOW_MASK_CONTROL_OUT:
	case CWINDOW_MASK_TRANSLATE:
		mwindow_global->undo->update_undo(_("mask point"), LOAD_AUTOMATION);
		break;
	}

	gui->current_operation = CWINDOW_NONE;
	return result;
}

void CWindowCanvas::zoom_resize_window(double percentage)
{
	int canvas_w, canvas_h;
	int new_w, new_h;

	calculate_sizes(edlsession->output_w, edlsession->output_h,
		percentage, canvas_w, canvas_h);

	new_w = canvas_w + (gui->get_w() - theme_global->ccanvas_w);
	new_h = canvas_h + (gui->get_h() - theme_global->ccanvas_h);
	gui->resize_window(new_w, new_h);
	gui->resize_event(new_w, new_h);
}

void CWindowCanvas::toggle_controls()
{
	mainsession->cwindow_controls = !mainsession->cwindow_controls;
	gui->resize_event(gui->get_w(), gui->get_h());
}

double CWindowCanvas::sample_aspect_ratio()
{
	if(edlsession->sample_aspect_ratio)
		return edlsession->sample_aspect_ratio;
	return 1.0;
}
