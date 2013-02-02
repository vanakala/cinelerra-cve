
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
#include "autos.h"
#include "bcsignals.h"
#include "canvas.h"
#include "clip.h"
#include "cpanel.h"
#include "cplayback.h"
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
#include "mwindowgui.h"
#include "mwindow.h"
#include "mwindow.h"
#include "playback3d.h"
#include "playtransport.h"
#include "theme.h"
#include "trackcanvas.h"
#include "tracks.h"
#include "transportcommand.h"
#include "vtrack.h"


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


CWindowGUI::CWindowGUI(MWindow *mwindow, CWindow *cwindow)
 : BC_Window(PROGRAM_NAME ": Compositor",
	mwindow->session->cwindow_x,
	mwindow->session->cwindow_y,
	mwindow->session->cwindow_w,
	mwindow->session->cwindow_h,
	100,
	100,
	1,
	1,
	1,
	BC_WindowBase::get_resources()->bg_color,
	mwindow->edl->session->get_cwindow_display())
{
	this->mwindow = mwindow;
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
	tool_panel = 0;
	translating_zoom = 0;
	active = 0;
	inactive = 0;
	crop_translate = 0;
}

CWindowGUI::~CWindowGUI()
{
	if(tool_panel) delete tool_panel;
	delete meters;
	delete composite_panel;
	delete canvas;
	delete transport;
	delete edit_panel;
	delete zoom_panel;
	delete active;
	delete inactive;
}

int CWindowGUI::create_objects()
{
	set_icon(mwindow->theme->get_image("cwindow_icon"));

	active = new BC_Pixmap(this, mwindow->theme->get_image("cwindow_active"));
	inactive = new BC_Pixmap(this, mwindow->theme->get_image("cwindow_inactive"));

	mwindow->theme->get_cwindow_sizes(this, mwindow->session->cwindow_controls);
	mwindow->theme->draw_cwindow_bg(this);
	flash();

// Meters required by composite panel
	meters = new CWindowMeters(mwindow, 
		this,
		mwindow->theme->cmeter_x,
		mwindow->theme->cmeter_y,
		mwindow->theme->cmeter_h);
	meters->create_objects();


	composite_panel = new CPanel(mwindow, 
		this, 
		mwindow->theme->ccomposite_x,
		mwindow->theme->ccomposite_y,
		mwindow->theme->ccomposite_w,
		mwindow->theme->ccomposite_h);
	composite_panel->create_objects();

	canvas = new CWindowCanvas(mwindow, this);
	canvas->create_objects(mwindow->edl);


	add_subwindow(timebar = new CTimeBar(mwindow,
		this,
		mwindow->theme->ctimebar_x,
		mwindow->theme->ctimebar_y,
		mwindow->theme->ctimebar_w, 
		mwindow->theme->ctimebar_h));
	timebar->create_objects();

	add_subwindow(slider = new CWindowSlider(mwindow, 
		cwindow, 
		mwindow->theme->cslider_x,
		mwindow->theme->cslider_y, 
		mwindow->theme->cslider_w));

	transport = new CWindowTransport(mwindow, 
		this, 
		mwindow->theme->ctransport_x, 
		mwindow->theme->ctransport_y);

	edit_panel = new CWindowEditing(mwindow, cwindow, meters);

	zoom_panel = new CWindowZoom(mwindow, 
		this, 
		mwindow->theme->czoom_x, 
		mwindow->theme->czoom_y);
	zoom_panel->create_objects();
	zoom_panel->zoom_text->add_item(new BC_MenuItem(AUTO_ZOOM));
	if(!mwindow->edl->session->cwindow_scrollbars) zoom_panel->set_text(AUTO_ZOOM);

// Must create after meter panel
	tool_panel = new CWindowTool(mwindow, this);
	tool_panel->Thread::start();

	set_operation(mwindow->edl->session->cwindow_operation);

	canvas->draw_refresh();
	draw_status();

	return 0;
}

void CWindowGUI::translation_event()
{
	mwindow->session->cwindow_x = get_x();
	mwindow->session->cwindow_y = get_y();
}

void CWindowGUI::resize_event(int w, int h)
{
	mwindow->session->cwindow_x = get_x();
	mwindow->session->cwindow_y = get_y();
	mwindow->session->cwindow_w = w;
	mwindow->session->cwindow_h = h;

	mwindow->theme->get_cwindow_sizes(this, mwindow->session->cwindow_controls);
	mwindow->theme->draw_cwindow_bg(this);
	flash();

	composite_panel->reposition_buttons(mwindow->theme->ccomposite_x,
		mwindow->theme->ccomposite_y);

	canvas->reposition_window(mwindow->edl,
		mwindow->theme->ccanvas_x,
		mwindow->theme->ccanvas_y,
		mwindow->theme->ccanvas_w,
		mwindow->theme->ccanvas_h);

	timebar->resize_event();

	slider->reposition_window(mwindow->theme->cslider_x,
		mwindow->theme->cslider_y, 
		mwindow->theme->cslider_w);
// Recalibrate pointer motion range
	slider->set_position();

	transport->reposition_buttons(mwindow->theme->ctransport_x, 
		mwindow->theme->ctransport_y);

	edit_panel->reposition_buttons(mwindow->theme->cedit_x, 
		mwindow->theme->cedit_y);

	zoom_panel->reposition_window(mwindow->theme->czoom_x, 
		mwindow->theme->czoom_y);

	meters->reposition_window(mwindow->theme->cmeter_x,
		mwindow->theme->cmeter_y,
		mwindow->theme->cmeter_h);

	draw_status();

	BC_WindowBase::resize_event(w, h);
}

int CWindowGUI::button_press_event()
{
	if(canvas->get_canvas())
		return canvas->button_press_event_base(canvas->get_canvas());
	return 0;
}

void CWindowGUI::cursor_leave_event()
{
	if(canvas->get_canvas())
		canvas->cursor_leave_event_base(canvas->get_canvas());
}

int CWindowGUI::cursor_enter_event()
{
	if(canvas->get_canvas())
		return canvas->cursor_enter_event_base(canvas->get_canvas());
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
	{
		draw_pixmap(active, 
			mwindow->theme->cstatus_x, 
			mwindow->theme->cstatus_y);
	}
	else
	{
		draw_pixmap(inactive, 
			mwindow->theme->cstatus_x, 
			mwindow->theme->cstatus_y);
	}
	
	flash(mwindow->theme->cstatus_x,
		mwindow->theme->cstatus_y,
		active->get_w(),
		active->get_h());
}


void CWindowGUI::zoom_canvas(int do_auto, double value, int update_menu)
{
	if(do_auto)
		mwindow->edl->session->cwindow_scrollbars = 0;
	else
		mwindow->edl->session->cwindow_scrollbars = 1;

	float old_zoom = mwindow->edl->session->cwindow_zoom;
	float new_zoom = value;
	float x = canvas->w / 2;
	float y = canvas->h / 2;
	canvas->canvas_to_output(mwindow->edl, 
				0, 
				x, 
				y);
	x -= canvas->w_visible / 2 * old_zoom / new_zoom;
	y -= canvas->h_visible / 2 * old_zoom / new_zoom;
	if(update_menu)
	{
		if(do_auto)
		{
			zoom_panel->update(AUTO_ZOOM);
		}
		else
		{
			zoom_panel->update(value);
		}
	}

	canvas->update_zoom((int)x, 
		(int)y, 
		new_zoom);
	canvas->reposition_window(mwindow->edl, 
		mwindow->theme->ccanvas_x,
		mwindow->theme->ccanvas_y,
		mwindow->theme->ccanvas_w,
		mwindow->theme->ccanvas_h);
	canvas->draw_refresh();
}


// TODO
// Don't refresh the canvas in a load file operation which is going to
// refresh it anyway.
void CWindowGUI::set_operation(int value)
{
	mwindow->edl->session->cwindow_operation = value;

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
			if(mwindow->session->cwindow_fullscreen)
				canvas->stop_fullscreen();
			else
				canvas->start_fullscreen();
			break;
		case ESC:
			if(mwindow->session->cwindow_fullscreen)
				canvas->stop_fullscreen();
			break;
		case LEFT:
			if(!ctrl_down()) 
			{ 
				if (alt_down())
				{
					int shift_down = this->shift_down();
					mwindow->gui->mbuttons->transport->handle_transport(STOP, 1, 0);

					mwindow->gui->lock_window("CWindowGUI::keypress_event 2");
					mwindow->prev_edit_handle(shift_down);
					mwindow->gui->unlock_window();

				}
				else
				{
					mwindow->move_left(); 
				}
 				result = 1; 
			}
			break;
		case RIGHT:
			if(!ctrl_down()) 
			{ 
				if (alt_down())
				{
					int shift_down = this->shift_down();
					mwindow->gui->mbuttons->transport->handle_transport(STOP, 1, 0);

					mwindow->gui->lock_window("CWindowGUI::keypress_event 2");
					mwindow->next_edit_handle(shift_down);
					mwindow->gui->unlock_window();

				}
				else
					mwindow->move_right(); 
				result = 1; 
			}
			break;
	}

	if(!result) result = transport->keypress_event();

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
	if(get_hidden()) return;

	if(mwindow->session->current_operation == DRAG_ASSET ||
		mwindow->session->current_operation == DRAG_VTRANSITION ||
		mwindow->session->current_operation == DRAG_VEFFECT)
	{
		int old_status = mwindow->session->ccanvas_highlighted;
		int cursor_x = get_relative_cursor_x();
		int cursor_y = get_relative_cursor_y();

		mwindow->session->ccanvas_highlighted = get_cursor_over_window() &&
			cursor_x >= canvas->x &&
			cursor_x < canvas->x + canvas->w &&
			cursor_y >= canvas->y &&
			cursor_y < canvas->y + canvas->h;


		if(old_status != mwindow->session->ccanvas_highlighted)
			canvas->draw_refresh();
	}
}

int CWindowGUI::drag_stop()
{
	int result = 0;
	if(get_hidden()) return 0;

	if((mwindow->session->current_operation == DRAG_ASSET ||
		mwindow->session->current_operation == DRAG_VTRANSITION ||
		mwindow->session->current_operation == DRAG_VEFFECT) &&
		mwindow->session->ccanvas_highlighted)
	{
// Hide highlighting
		mwindow->session->ccanvas_highlighted = 0;
		canvas->draw_refresh();
		result = 1;
	}
	else
		return 0;

	if(mwindow->session->current_operation == DRAG_ASSET)
	{
		if(mwindow->session->drag_assets->total)
		{
			mwindow->clear(0);
			mwindow->load_assets(mwindow->session->drag_assets, 
				mwindow->edl->local_session->get_selectionstart(), 
				LOAD_PASTE,
				mwindow->session->track_highlighted,
				0,
				mwindow->edl->session->edit_actions(),
				0); // overwrite
		}

		if(mwindow->session->drag_clips->total)
		{
			mwindow->clear(0);
			mwindow->paste_edls(mwindow->session->drag_clips, 
				LOAD_PASTE, 
				mwindow->session->track_highlighted,
				mwindow->edl->local_session->get_selectionstart(),
				mwindow->edl->session->edit_actions(),
				0); // overwrite
		}

		if(mwindow->session->drag_assets->total ||
			mwindow->session->drag_clips->total)
		{
			mwindow->save_backup();
			mwindow->restart_brender();
			mwindow->gui->update(1, 1, 1, 1, 0, 1, 0);
			mwindow->undo->update_undo(_("insert assets"), LOAD_ALL);
			mwindow->sync_parameters(LOAD_ALL);
		}
	}

	if(mwindow->session->current_operation == DRAG_VEFFECT)
	{
		Track *affected_track = cwindow->calculate_affected_track();

		mwindow->gui->lock_window("CWindowGUI::drag_stop 3");
		mwindow->insert_effects_cwindow(affected_track);
		mwindow->session->current_operation = NO_OPERATION;
		mwindow->gui->unlock_window();
	}

	if(mwindow->session->current_operation == DRAG_VTRANSITION)
	{
		Track *affected_track = cwindow->calculate_affected_track();
		mwindow->gui->lock_window("CWindowGUI::drag_stop 4");
		mwindow->paste_transition_cwindow(affected_track);
		mwindow->session->current_operation = NO_OPERATION;
		mwindow->gui->unlock_window();
	}

	return result;
}


CWindowEditing::CWindowEditing(MWindow *mwindow, CWindow *cwindow, MeterPanel *meter_panel)
 : EditPanel(mwindow, 
		cwindow->gui, 
		mwindow->theme->cedit_x, 
		mwindow->theme->cedit_y,
		EDTP_KEYFRAME | EDTP_COPY | EDTP_PASTE | EDTP_UNDO
			| EDTP_LABELS | EDTP_TOCLIP | EDTP_CUT,
		meter_panel)
{
	this->mwindow = mwindow;
	this->cwindow = cwindow;
}

void CWindowEditing::set_inpoint()
{
	mwindow->set_inpoint(0);
}

void CWindowEditing::set_outpoint()
{
	mwindow->set_outpoint(0);
}





CWindowMeters::CWindowMeters(MWindow *mwindow, CWindowGUI *gui, int x, int y, int h)
 : MeterPanel(mwindow, 
		gui,
		x,
		y,
		h,
		mwindow->edl->session->audio_channels,
		mwindow->edl->session->cwindow_meter)
{
	this->mwindow = mwindow;
	this->gui = gui;
}

CWindowMeters::~CWindowMeters()
{
}

int CWindowMeters::change_status_event()
{
	mwindow->edl->session->cwindow_meter = use_meters;
	mwindow->theme->get_cwindow_sizes(gui, mwindow->session->cwindow_controls);
	gui->resize_event(gui->get_w(), gui->get_h());
	return 1;
}




CWindowZoom::CWindowZoom(MWindow *mwindow, CWindowGUI *gui, int x, int y)
 : ZoomPanel(mwindow, 
	gui, 
	(double)mwindow->edl->session->cwindow_zoom, 
	x, 
	y,
	80, 
	my_zoom_table, 
	total_zooms, 
	ZOOM_PERCENTAGE)
{
	this->mwindow = mwindow;
	this->gui = gui;
}

CWindowZoom::~CWindowZoom()
{
}

int CWindowZoom::handle_event()
{
	if(!strcasecmp(AUTO_ZOOM, get_text()))
	{
		gui->zoom_canvas(1, get_value(), 0);
	}
	else
	{
		gui->zoom_canvas(0, get_value(), 0);
	}

	return 1;
}



CWindowSlider::CWindowSlider(MWindow *mwindow, CWindow *cwindow, int x, int y, int pixels)
 : BC_PercentageSlider(x, 
			y,
			0,
			pixels, 
			pixels, 
			0, 
			1, 
			0)
{
	this->mwindow = mwindow;
	this->cwindow = cwindow;
	set_precision(0.00001);
	set_pagination(1.0, 10.0);
}

CWindowSlider::~CWindowSlider()
{
}

int CWindowSlider::handle_event()
{
	cwindow->playback_engine->interrupt_playback(1);

	mwindow->gui->lock_window("CWindowSlider::handle_event 2");
	mwindow->select_point((double)get_value());
	mwindow->gui->unlock_window();
	return 1;
}

void CWindowSlider::set_position()
{
	double new_length = mwindow->edl->tracks->total_playable_length();
	if(mwindow->edl->local_session->preview_end <= 0 ||
		mwindow->edl->local_session->preview_end > new_length)
		mwindow->edl->local_session->preview_end = new_length;
	if(mwindow->edl->local_session->preview_start > 
		mwindow->edl->local_session->preview_end)
		mwindow->edl->local_session->preview_start = 0;



	update(mwindow->theme->cslider_w, 
		mwindow->edl->local_session->get_selectionstart(1), 
		mwindow->edl->local_session->preview_start, 
		mwindow->edl->local_session->preview_end);
}


void CWindowSlider::increase_value()
{
	cwindow->gui->transport->handle_transport(SINGLE_FRAME_FWD);
}

void CWindowSlider::decrease_value()
{
	cwindow->gui->transport->handle_transport(SINGLE_FRAME_REWIND);
}


CWindowTransport::CWindowTransport(MWindow *mwindow, 
	CWindowGUI *gui, 
	int x, 
	int y)
 : PlayTransport(mwindow, 
	gui, 
	x, 
	y)
{
	this->gui = gui;
}

EDL* CWindowTransport::get_edl()
{
	return mwindow->edl;
}

void CWindowTransport::goto_start()
{
	handle_transport(REWIND, 1);

	mwindow->gui->lock_window("CWindowTransport::goto_start 1");
	mwindow->goto_start();
	mwindow->gui->unlock_window();

}

void CWindowTransport::goto_end()
{
	handle_transport(GOTO_END, 1);

	mwindow->gui->lock_window("CWindowTransport::goto_end 1");
	mwindow->goto_end();
	mwindow->gui->unlock_window();
}



CWindowCanvas::CWindowCanvas(MWindow *mwindow, CWindowGUI *gui)
 : Canvas(mwindow,
	gui,
	mwindow->theme->ccanvas_x,
	mwindow->theme->ccanvas_y,
	mwindow->theme->ccanvas_w,
	mwindow->theme->ccanvas_h,
	0,
	0,
	mwindow->edl->session->cwindow_scrollbars,
	1)
{
	this->mwindow = mwindow;
	this->gui = gui;
}

void CWindowCanvas::status_event()
{
	gui->draw_status();
}

int CWindowCanvas::get_fullscreen()
{
	return mwindow->session->cwindow_fullscreen;
}

void CWindowCanvas::set_fullscreen(int value)
{
	mwindow->session->cwindow_fullscreen = value;
}


void CWindowCanvas::update_zoom(int x, int y, float zoom)
{
	use_scrollbars = mwindow->edl->session->cwindow_scrollbars;

	mwindow->edl->session->cwindow_xscroll = x;
	mwindow->edl->session->cwindow_yscroll = y;
	mwindow->edl->session->cwindow_zoom = zoom;
}

void CWindowCanvas::zoom_auto()
{
	gui->zoom_canvas(1, 1.0, 1);
}

int CWindowCanvas::get_xscroll()
{
	return mwindow->edl->session->cwindow_xscroll;
}

int CWindowCanvas::get_yscroll()
{
	return mwindow->edl->session->cwindow_yscroll;
}


float CWindowCanvas::get_zoom()
{
	return mwindow->edl->session->cwindow_zoom;
}

void CWindowCanvas::draw_refresh()
{
	if(get_canvas() && !get_canvas()->get_video_on())
	{

		if(refresh_frame)
		{
			float in_x1, in_y1, in_x2, in_y2;
			float out_x1, out_y1, out_x2, out_y2;
			get_transfers(mwindow->edl, 
				in_x1, 
				in_y1, 
				in_x2, 
				in_y2, 
				out_x1, 
				out_y1, 
				out_x2, 
				out_y2);

			get_canvas()->clear_box(0, 
				0, 
				get_canvas()->get_w(), 
				get_canvas()->get_h());

			if(out_x2 > out_x1 && 
				out_y2 > out_y1 && 
				in_x2 > in_x1 && 
				in_y2 > in_y1)
			{
// Can't use OpenGL here because it is called asynchronously of the
// playback operation.
				get_canvas()->draw_vframe(refresh_frame,
						(int)out_x1, 
						(int)out_y1, 
						(int)(out_x2 - out_x1), 
						(int)(out_y2 - out_y1),
						(int)in_x1, 
						(int)in_y1, 
						(int)(in_x2 - in_x1), 
						(int)(in_y2 - in_y1),
						0);
			}
		}

		draw_overlays();
		get_canvas()->flash();
	}
}

#define CROPHANDLE_W 10
#define CROPHANDLE_H 10

void CWindowCanvas::draw_crophandle(int x, int y)
{
	get_canvas()->draw_box(x, y, CROPHANDLE_W, CROPHANDLE_H);
}

#define CONTROL_W 10
#define CONTROL_H 10
#define FIRST_CONTROL_W 20
#define FIRST_CONTROL_H 20
#undef BC_INFINITY
#define BC_INFINITY 65536
#ifndef SQR
#define SQR(x) ((x) * (x))
#endif

int CWindowCanvas::do_mask(int &redraw, 
		int &rerender, 
		int button_press, 
		int cursor_motion,
		int draw)
{
// Retrieve points from top recordable track
	Track *track = gui->cwindow->calculate_affected_track();

	if(!track) return 0;

	MaskAutos *mask_autos = (MaskAutos*)track->automation->autos[AUTOMATION_MASK];
	ptstime position = mwindow->edl->local_session->get_selectionstart(1);

	ArrayList<MaskPoint*> points;
	mask_autos->get_points(&points, mwindow->edl->session->cwindow_mask,
		position);

// Projector zooms relative to the center of the track output.
	float half_track_w = (float)track->track_w / 2;
	float half_track_h = (float)track->track_h / 2;
// Translate mask to projection
	float projector_x, projector_y, projector_z;
	track->automation->get_projector(&projector_x,
		&projector_y,
		&projector_z,
		position);


// Get position of cursor relative to mask
	float mask_cursor_x = get_cursor_x();
	float mask_cursor_y = get_cursor_y();
	canvas_to_output(mwindow->edl, 0, mask_cursor_x, mask_cursor_y);

	projector_x += mwindow->edl->session->output_w / 2;
	projector_y += mwindow->edl->session->output_h / 2;

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
	float shortest_line_distance = BC_INFINITY;
// Distance to closest point
	float shortest_point_distance = BC_INFINITY;
	int selected_point = -1;
	int selected_control_point = -1;
	float selected_control_point_distance = BC_INFINITY;
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
			float x0, x1, x2, x3;
			float y0, y1, y2, y3;
			float old_x, old_y, x, y;
			int segments = (int)(sqrt(SQR(point1->x - point2->x) + SQR(point1->y - point2->y)));

			for(int j = 0; j <= segments && !result; j++)
			{
				x0 = point1->x;
				y0 = point1->y;
				x1 = point1->x + point1->control_x2;
				y1 = point1->y + point1->control_y2;
				x2 = point2->x + point2->control_x1;
				y2 = point2->y + point2->control_y1;
				x3 = point2->x;
				y3 = point2->y;

				float t = (float)j / segments;
				float tpow2 = t * t;
				float tpow3 = t * t * t;
				float invt = 1 - t;
				float invtpow2 = invt * invt;
				float invtpow3 = invt * invt * invt;

				x = (        invtpow3 * x0
					+ 3 * t     * invtpow2 * x1
					+ 3 * tpow2 * invt     * x2 
					+     tpow3            * x3);
				y = (        invtpow3 * y0 
					+ 3 * t     * invtpow2 * y1
					+ 3 * tpow2 * invt     * y2 
					+     tpow3            * y3);

				x = (x - half_track_w) * projector_z + projector_x;
				y = (y - half_track_h) * projector_z + projector_y;


// Test new point addition
				if(button_press)
				{
					float line_distance = 
						sqrt(SQR(x - mask_cursor_x) + SQR(y - mask_cursor_y));

					if(line_distance < shortest_line_distance || 
						shortest_point1 < 0)
					{
						shortest_line_distance = line_distance;
						shortest_point1 = i;
						shortest_point2 = (i >= points.total - 1) ? 0 : (i + 1);
					}


					float point_distance1 = 
						sqrt(SQR(point1->x - mask_cursor_x) + SQR(point1->y - mask_cursor_y));
					float point_distance2 = 
						sqrt(SQR(point2->x - mask_cursor_x) + SQR(point2->y - mask_cursor_y));

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

				output_to_canvas(mwindow->edl, 0, x, y);


#define TEST_BOX(cursor_x, cursor_y, target_x, target_y) \
	(cursor_x >= target_x - CONTROL_W / 2 && \
	cursor_x < target_x + CONTROL_W / 2 && \
	cursor_y >= target_y - CONTROL_H / 2 && \
	cursor_y < target_y + CONTROL_H / 2)

// Test existing point selection
				if(button_press)
				{
					float canvas_x = (x0 - half_track_w) * projector_z + projector_x;
					float canvas_y = (y0 - half_track_h) * projector_z + projector_y;
					int cursor_x = get_cursor_x();
					int cursor_y = get_cursor_y();

// Test first point
					if(gui->shift_down())
					{
						float control_x = (x1 - half_track_w) * projector_z + projector_x;
						float control_y = (y1 - half_track_h) * projector_z + projector_y;
						output_to_canvas(mwindow->edl, 0, control_x, control_y);

						float distance = 
							sqrt(SQR(control_x - cursor_x) + SQR(control_y - cursor_y));

						if(distance < selected_control_point_distance)
						{
							selected_point = i;
							selected_control_point = 1;
							selected_control_point_distance = distance;
						}
					}
					else
					{
						output_to_canvas(mwindow->edl, 0, canvas_x, canvas_y);
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
					canvas_x = (x3 - half_track_w) * projector_z + projector_x;
					canvas_y = (y3 - half_track_h) * projector_z + projector_y;
					if(gui->shift_down())
					{
						float control_x = (x2 - half_track_w) * projector_z + projector_x;
						float control_y = (y2 - half_track_h) * projector_z + projector_y;
						output_to_canvas(mwindow->edl, 0, control_x, control_y);

						float distance = 
							sqrt(SQR(control_x - cursor_x) + SQR(control_y - cursor_y));

						if(distance < selected_control_point_distance)
						{
							selected_point = (i < points.total - 1 ? i + 1 : 0);
							selected_control_point = 0;
							selected_control_point_distance = distance;
						}
					}
					else
					if(i < points.total - 1)
					{
						output_to_canvas(mwindow->edl, 0, canvas_x, canvas_y);
						if(!gui->ctrl_down())
						{
							if(TEST_BOX(cursor_x, cursor_y, canvas_x, canvas_y))
							{
								selected_point = (i < points.total - 1 ? i + 1 : 0);
							}
						}
						else
						{
							selected_point = shortest_point;
						}
					}
				}



				if(j > 0)
				{
// Draw joining line
					if(draw)
					{
						x_points.append((int)x);
						y_points.append((int)y);
					}

					if(j == segments)
					{
						if(draw)
						{
// Draw second anchor
							if(i < points.total - 1)
							{
								if(i == gui->affected_point - 1)
									get_canvas()->draw_disc((int)x - CONTROL_W / 2, 
										(int)y - CONTROL_W / 2, 
										CONTROL_W, 
										CONTROL_W);
								else
									get_canvas()->draw_circle((int)x - CONTROL_W / 2, 
										(int)y - CONTROL_W / 2, 
										CONTROL_W, 
										CONTROL_W);
							}

// Draw second control point.  Discard x2 and y2 after this.
							x2 = (x2 - half_track_w) * projector_z + projector_x;
							y2 = (y2 - half_track_h) * projector_z + projector_y;
							output_to_canvas(mwindow->edl, 0, x2, y2);
							get_canvas()->draw_line((int)x, (int)y, (int)x2, (int)y2);
							get_canvas()->draw_rectangle((int)x2 - CONTROL_W / 2,
								(int)y2 - CONTROL_H / 2,
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
						get_canvas()->draw_disc((int)x - FIRST_CONTROL_W / 2, 
							(int)y - FIRST_CONTROL_H / 2, 
							FIRST_CONTROL_W, 
							FIRST_CONTROL_H);
					}

// Draw first control point.  Discard x1 and y1 after this.
					if(draw)
					{
						x1 = (x1 - half_track_w) * projector_z + projector_x;
						y1 = (y1 - half_track_h) * projector_z + projector_y;
						output_to_canvas(mwindow->edl, 0, x1, y1);
						get_canvas()->draw_line((int)x, (int)y, (int)x1, (int)y1);
						get_canvas()->draw_rectangle((int)x1 - CONTROL_W / 2,
							(int)y1 - CONTROL_H / 2,
							CONTROL_W,
							CONTROL_H);
						x_points.append((int)x);
						y_points.append((int)y);
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
				gui->cwindow->calculate_affected_auto(
					gui->affected_track->automation->autos[AUTOMATION_MASK],
					1);

		MaskAuto *keyframe = (MaskAuto*)gui->affected_keyframe;
		SubMask *mask = keyframe->get_submask(mwindow->edl->session->cwindow_mask);


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
				gui->current_operation = mwindow->edl->session->cwindow_operation;
		}
		else
// No existing point or control point was selected so create a new one
		if(!gui->shift_down() && !gui->alt_down())
		{
// Create the template
			MaskPoint *point = new MaskPoint;
			point->x = mask_cursor_x;
			point->y = mask_cursor_y;
			point->control_x1 = 0;
			point->control_y1 = 0;
			point->control_x2 = 0;
			point->control_y2 = 0;


			if(shortest_point2 < shortest_point1)
			{
				shortest_point2 ^= shortest_point1;
				shortest_point1 ^= shortest_point2;
				shortest_point2 ^= shortest_point1;
			}

// Append to end of list
			if(labs(shortest_point1 - shortest_point2) > 1)
			{
// Need to apply the new point to every keyframe
				for(MaskAuto *current = (MaskAuto*)mask_autos->default_auto;
					current; )
				{
					SubMask *submask = current->get_submask(mwindow->edl->session->cwindow_mask);
					MaskPoint *new_point = new MaskPoint;
					submask->points.append(new_point);
					*new_point = *point;
					if(current == (MaskAuto*)mask_autos->default_auto)
						current = (MaskAuto*)mask_autos->first;
					else
						current = (MaskAuto*)NEXT;
				}

				gui->affected_point = mask->points.total - 1;
				result = 1;
			}
			else
// Insert between 2 points, shifting back point 2
			if(shortest_point1 >= 0 && shortest_point2 >= 0)
			{
				for(MaskAuto *current = (MaskAuto*)mask_autos->default_auto;
					current; )
				{
					SubMask *submask = current->get_submask(mwindow->edl->session->cwindow_mask);
// In case the keyframe point count isn't synchronized with the rest of the keyframes,
// avoid a crash.
					if(submask->points.total >= shortest_point2)
					{
						MaskPoint *new_point = new MaskPoint;
						submask->points.append(0);
						for(int i = submask->points.total - 1; 
							i > shortest_point2; 
							i--)
							submask->points.values[i] = submask->points.values[i - 1];
						submask->points.values[shortest_point2] = new_point;

						*new_point = *point;
					}

					if(current == (MaskAuto*)mask_autos->default_auto)
						current = (MaskAuto*)mask_autos->first;
					else
						current = (MaskAuto*)NEXT;
				}


				gui->affected_point = shortest_point2;
				result = 1;
			}

// Create the first point.
			if(!result)
			{
				for(MaskAuto *current = (MaskAuto*)mask_autos->default_auto;
					current; )
				{
					SubMask *submask = current->get_submask(mwindow->edl->session->cwindow_mask);
					MaskPoint *new_point = new MaskPoint;
					submask->points.append(new_point);
					*new_point = *point;
					if(current == (MaskAuto*)mask_autos->default_auto)
						current = (MaskAuto*)mask_autos->first;
					else
						current = (MaskAuto*)NEXT;
				}

				if(mask->points.total < 2)
				{
					for(MaskAuto *current = (MaskAuto*)mask_autos->default_auto;
						current; )
					{
						SubMask *submask = current->get_submask(mwindow->edl->session->cwindow_mask);
						MaskPoint *new_point = new MaskPoint;
						submask->points.append(new_point);
						*new_point = *point;
						if(current == (MaskAuto*)mask_autos->default_auto)
							current = (MaskAuto*)mask_autos->first;
						else
							current = (MaskAuto*)NEXT;
					}
				}
				gui->affected_point = mask->points.total - 1;
			}

			gui->current_operation = mwindow->edl->session->cwindow_operation;
// Delete the template
			delete point;
			mwindow->undo->update_undo(_("mask point"), LOAD_AUTOMATION);
		}

		result = 1;
		rerender = 1;
		redraw = 1;
	}

	if(button_press && result)
	{
		MaskAuto *keyframe = (MaskAuto*)gui->affected_keyframe;
		SubMask *mask = keyframe->get_submask(mwindow->edl->session->cwindow_mask);
		MaskPoint *point = mask->points.values[gui->affected_point];
		gui->center_x = point->x;
		gui->center_y = point->y;
		gui->control_in_x = point->control_x1;
		gui->control_in_y = point->control_y1;
		gui->control_out_x = point->control_x2;
		gui->control_out_y = point->control_y2;
	}

	if(cursor_motion)
	{
		MaskAuto *keyframe = (MaskAuto*)gui->affected_keyframe;
		SubMask *mask = keyframe->get_submask(mwindow->edl->session->cwindow_mask);
		if(gui->affected_point < mask->points.total)
		{
			MaskPoint *point = mask->points.values[gui->affected_point];
			float cursor_x = mask_cursor_x;
			float cursor_y = mask_cursor_y;

			float last_x = point->x;
			float last_y = point->y;
			float last_control_x1 = point->control_x1;
			float last_control_y1 = point->control_y1;
			float last_control_x2 = point->control_x2;
			float last_control_y2 = point->control_y2;


			switch(gui->current_operation)
			{
				case CWINDOW_MASK:
					point->x = cursor_x - gui->x_origin + gui->center_x;
					point->y = cursor_y - gui->y_origin + gui->center_y;
					break;

				case CWINDOW_MASK_CONTROL_IN:
					point->control_x1 = cursor_x - gui->x_origin + gui->control_in_x;
					point->control_y1 = cursor_y - gui->y_origin + gui->control_in_y;
					break;

				case CWINDOW_MASK_CONTROL_OUT:
					point->control_x2 = cursor_x - gui->x_origin + gui->control_out_x;
					point->control_y2 = cursor_y - gui->y_origin + gui->control_out_y;
					break;

				case CWINDOW_MASK_TRANSLATE:
					for(int i = 0; i < mask->points.total; i++)
					{
						mask->points.values[i]->x += cursor_x - gui->x_origin;
						mask->points.values[i]->y += cursor_y - gui->y_origin;
					}
					gui->x_origin = cursor_x;
					gui->y_origin = cursor_y;
					break;
			}


			if( !EQUIV(last_x, point->x) ||
				!EQUIV(last_y, point->y) ||
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
	float cursor_x = get_cursor_x();
	float cursor_y = get_cursor_y();


	if(button_press)
	{
		gui->current_operation = CWINDOW_EYEDROP;
	}

	if(gui->current_operation == CWINDOW_EYEDROP)
	{
		canvas_to_output(mwindow->edl, 0, cursor_x, cursor_y);

// Get color out of frame.
// Doesn't work during playback because that bypasses the refresh frame.
		if(refresh_frame)
		{
			CLAMP(cursor_x, 0, refresh_frame->get_w() - 1);
			CLAMP(cursor_y, 0, refresh_frame->get_h() - 1);

// Decompression coefficients straight out of jpeglib
#define V_TO_R    1.40200
#define V_TO_G    -0.71414

#define U_TO_G    -0.34414
#define U_TO_B    1.77200

#define GET_COLOR(type, components, max, do_yuv) \
{ \
	type *row = (type*)(refresh_frame->get_rows()[(int)cursor_y]) + \
		(int)cursor_x * components; \
	float red = (float)*row++ / max; \
	float green = (float)*row++ / max; \
	float blue = (float)*row++ / max; \
	if(do_yuv) \
	{ \
		mwindow->edl->local_session->red = red + V_TO_R * (blue - 0.5); \
		mwindow->edl->local_session->green = red + U_TO_G * (green - 0.5) + V_TO_G * (blue - 0.5); \
		mwindow->edl->local_session->blue = red + U_TO_B * (green - 0.5); \
	} \
	else \
	{ \
		mwindow->edl->local_session->red = red; \
		mwindow->edl->local_session->green = green; \
		mwindow->edl->local_session->blue = blue; \
	} \
}

			switch(refresh_frame->get_color_model())
			{
				case BC_YUV888:
					GET_COLOR(unsigned char, 3, 0xff, 1);
					break;
				case BC_YUVA8888:
					GET_COLOR(unsigned char, 4, 0xff, 1);
					break;
				case BC_YUV161616:
					GET_COLOR(uint16_t, 3, 0xffff, 1);
					break;
				case BC_YUVA16161616:
					GET_COLOR(uint16_t, 4, 0xffff, 1);
					break;
				case BC_RGB888:
					GET_COLOR(unsigned char, 3, 0xff, 0);
					break;
				case BC_RGBA8888:
					GET_COLOR(unsigned char, 4, 0xff, 0);
					break;
				case BC_RGB_FLOAT:
					GET_COLOR(float, 3, 1.0, 0);
					break;
				case BC_RGBA_FLOAT:
					GET_COLOR(float, 4, 1.0, 0);
					break;
			}
		}
		else
		{
			mwindow->edl->local_session->red = 0;
			mwindow->edl->local_session->green = 0;
			mwindow->edl->local_session->blue = 0;
		}

		gui->update_tool();
		result = 1;
// Can't rerender since the color value is from the output of any effect it
// goes into.
//		rerender = 1;
	}

	return result;
}

void CWindowCanvas::draw_overlays()
{
	if(mwindow->edl->session->safe_regions)
	{
		draw_safe_regions();
	}

	if(mwindow->edl->session->cwindow_scrollbars)
	{
// Always draw output rectangle
		float x1, y1, x2, y2;
		x1 = 0;
		x2 = mwindow->edl->session->output_w;
		y1 = 0;
		y2 = mwindow->edl->session->output_h;
		output_to_canvas(mwindow->edl, 0, x1, y1);
		output_to_canvas(mwindow->edl, 0, x2, y2);

		get_canvas()->set_inverse();
		get_canvas()->set_color(WHITE);

		get_canvas()->draw_rectangle((int)x1, 
				(int)y1, 
				(int)(x2 - x1), 
				(int)(y2 - y1));

		get_canvas()->set_opaque();
	}

	if(mwindow->session->ccanvas_highlighted)
	{
		get_canvas()->set_color(WHITE);
		get_canvas()->set_inverse();
		get_canvas()->draw_rectangle(0, 0, get_canvas()->get_w(), get_canvas()->get_h());
		get_canvas()->draw_rectangle(1, 1, get_canvas()->get_w() - 2, get_canvas()->get_h() - 2);
		get_canvas()->set_opaque();
	}

	int temp1 = 0, temp2 = 0;
	switch(mwindow->edl->session->cwindow_operation)
	{
		case CWINDOW_CAMERA:
			draw_bezier(1);
			break;

		case CWINDOW_PROJECTOR:
			draw_bezier(0);
			break;

		case CWINDOW_CROP:
			draw_crop();
			break;

		case CWINDOW_MASK:
			do_mask(temp1, temp2, 0, 0, 1);
			break;
	}
}

void CWindowCanvas::draw_safe_regions()
{
	float action_x1, action_x2, action_y1, action_y2;
	float title_x1, title_x2, title_y1, title_y2;

	action_x1 = mwindow->edl->session->output_w / 2 - mwindow->edl->session->output_w / 2 * 0.9;
	action_x2 = mwindow->edl->session->output_w / 2 + mwindow->edl->session->output_w / 2 * 0.9;
	action_y1 = mwindow->edl->session->output_h / 2 - mwindow->edl->session->output_h / 2 * 0.9;
	action_y2 = mwindow->edl->session->output_h / 2 + mwindow->edl->session->output_h / 2 * 0.9;
	title_x1 = mwindow->edl->session->output_w / 2 - mwindow->edl->session->output_w / 2 * 0.8;
	title_x2 = mwindow->edl->session->output_w / 2 + mwindow->edl->session->output_w / 2 * 0.8;
	title_y1 = mwindow->edl->session->output_h / 2 - mwindow->edl->session->output_h / 2 * 0.8;
	title_y2 = mwindow->edl->session->output_h / 2 + mwindow->edl->session->output_h / 2 * 0.8;

	output_to_canvas(mwindow->edl, 0, action_x1, action_y1);
	output_to_canvas(mwindow->edl, 0, action_x2, action_y2);
	output_to_canvas(mwindow->edl, 0, title_x1, title_y1);
	output_to_canvas(mwindow->edl, 0, title_x2, title_y2);

	get_canvas()->set_inverse();
	get_canvas()->set_color(WHITE);

	get_canvas()->draw_rectangle((int)action_x1, 
			(int)action_y1, 
			(int)(action_x2 - action_x1), 
			(int)(action_y2 - action_y1));
	get_canvas()->draw_rectangle((int)title_x1, 
			(int)title_y1, 
			(int)(title_x2 - title_x1), 
			(int)(title_y2 - title_y1));

	get_canvas()->set_opaque();
}

void CWindowCanvas::reset_keyframe(int do_camera)
{
	FloatAuto *x_keyframe = 0;
	FloatAuto *y_keyframe = 0;
	FloatAuto *z_keyframe = 0;
	Track *affected_track = 0;

	affected_track = gui->cwindow->calculate_affected_track();

	if(affected_track)
	{
		gui->cwindow->calculate_affected_autos(&x_keyframe,
			&y_keyframe,
			&z_keyframe,
			affected_track,
			do_camera,
			1,
			1,
			1);

		x_keyframe->value = 0;
		y_keyframe->value = 0;
		z_keyframe->value = 1;

		mwindow->sync_parameters(CHANGE_PARAMS);
		gui->update_tool();
	}
}

void CWindowCanvas::reset_camera()
{
	reset_keyframe(1);
}

void CWindowCanvas::reset_projector()
{
	reset_keyframe(0);
}

int CWindowCanvas::test_crop(int button_press, int &redraw)
{
	int result = 0;
	int handle_selected = -1;
	float x1 = mwindow->edl->session->crop_x1;
	float y1 = mwindow->edl->session->crop_y1;
	float x2 = mwindow->edl->session->crop_x2;
	float y2 = mwindow->edl->session->crop_y2;
	float cursor_x = get_cursor_x();
	float cursor_y = get_cursor_y();
	float canvas_x1 = x1;
	float canvas_y1 = y1;
	float canvas_x2 = x2;
	float canvas_y2 = y2;
	float canvas_cursor_x = cursor_x;
	float canvas_cursor_y = cursor_y;

	canvas_to_output(mwindow->edl, 0, cursor_x, cursor_y);
// Use screen normalized coordinates for hot spot tests.
	output_to_canvas(mwindow->edl, 0, canvas_x1, canvas_y1);
	output_to_canvas(mwindow->edl, 0, canvas_x2, canvas_y2);


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
	else
// Start new box
	{
		gui->crop_origin_x = cursor_x;
		gui->crop_origin_y = cursor_y;
	}

// Start dragging.
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
		{
			x2 = x1 = cursor_x;
			y2 = y1 = cursor_y;
			mwindow->edl->session->crop_x1 = (int)x1;
			mwindow->edl->session->crop_y1 = (int)y1;
			mwindow->edl->session->crop_x2 = (int)x2;
			mwindow->edl->session->crop_y2 = (int)y2;
			redraw = 1;
		}
	}
	else
// Translate all 4 points
	if(gui->current_operation == CWINDOW_CROP && gui->crop_translate)
	{
		x1 = cursor_x - gui->x_origin + gui->crop_origin_x1;
		y1 = cursor_y - gui->y_origin + gui->crop_origin_y1;
		x2 = cursor_x - gui->x_origin + gui->crop_origin_x2;
		y2 = cursor_y - gui->y_origin + gui->crop_origin_y2;

		mwindow->edl->session->crop_x1 = (int)x1;
		mwindow->edl->session->crop_y1 = (int)y1;
		mwindow->edl->session->crop_x2 = (int)x2;
		mwindow->edl->session->crop_y2 = (int)y2;
		result = 1;
		redraw = 1;
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

		if(!EQUIV(mwindow->edl->session->crop_x1, x1) ||
			!EQUIV(mwindow->edl->session->crop_x2, x2) ||
			!EQUIV(mwindow->edl->session->crop_y1, y1) ||
			!EQUIV(mwindow->edl->session->crop_y2, y2))
		{
			if (x1 > x2) 
			{
				float tmp = x1;
				x1 = x2;
				x2 = tmp;
				switch (gui->crop_handle) 
				{
				case 0:
					gui->crop_handle = 1; break;
				case 1:
					gui->crop_handle = 0; break;
				case 2:
					gui->crop_handle = 3; break;
				case 3:
					gui->crop_handle = 2; break;
				default:
					break;
				}

			}
			if (y1 > y2) 
			{
				float tmp = y1;
				y1 = y2;
				y2 = tmp;
				switch (gui->crop_handle) 
				{
				case 0:
					gui->crop_handle = 2; break;
				case 1:
					gui->crop_handle = 3; break;
				case 2:
					gui->crop_handle = 0; break;
				case 3:
					gui->crop_handle = 1; break;
				default:
					break;
				}
			}

			mwindow->edl->session->crop_x1 = (int)x1;
			mwindow->edl->session->crop_y1 = (int)y1;
			mwindow->edl->session->crop_x2 = (int)x2;
			mwindow->edl->session->crop_y2 = (int)y2;
			result = 1;
			redraw = 1;
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
	{
		set_cursor(ARROW_CURSOR);
	}
#define CLAMP(x, y, z) ((x) = ((x) < (y) ? (y) : ((x) > (z) ? (z) : (x))))

	if(redraw)
	{
		CLAMP(mwindow->edl->session->crop_x1, 0, mwindow->edl->session->output_w);
		CLAMP(mwindow->edl->session->crop_x2, 0, mwindow->edl->session->output_w);
		CLAMP(mwindow->edl->session->crop_y1, 0, mwindow->edl->session->output_h);
		CLAMP(mwindow->edl->session->crop_y2, 0, mwindow->edl->session->output_h);
	}
	return result;
}


void CWindowCanvas::draw_crop()
{
	get_canvas()->set_inverse();
	get_canvas()->set_color(WHITE);

	float x1 = mwindow->edl->session->crop_x1;
	float y1 = mwindow->edl->session->crop_y1;
	float x2 = mwindow->edl->session->crop_x2;
	float y2 = mwindow->edl->session->crop_y2;

	output_to_canvas(mwindow->edl, 0, x1, y1);
	output_to_canvas(mwindow->edl, 0, x2, y2);

	if(x2 - x1 && y2 - y1)
		get_canvas()->draw_rectangle((int)x1, 
			(int)y1, 
			(int)(x2 - x1), 
			(int)(y2 - y1));

	draw_crophandle((int)x1, (int)y1);
	draw_crophandle((int)x2 - CROPHANDLE_W, (int)y1);
	draw_crophandle((int)x1, (int)y2 - CROPHANDLE_H);
	draw_crophandle((int)x2 - CROPHANDLE_W, (int)y2 - CROPHANDLE_H);
	get_canvas()->set_opaque();
}



void CWindowCanvas::draw_bezier(int do_camera)
{
	Track *track = gui->cwindow->calculate_affected_track();

	if(!track) return;

	float center_x;
	float center_y;
	float center_z;
	ptstime position = mwindow->edl->local_session->get_selectionstart(1);

	track->automation->get_projector(&center_x, 
		&center_y, 
		&center_z, 
		position);

	center_x += mwindow->edl->session->output_w / 2;
	center_y += mwindow->edl->session->output_h / 2;
	float track_x1 = center_x - track->track_w / 2 * center_z;
	float track_y1 = center_y - track->track_h / 2 * center_z;
	float track_x2 = track_x1 + track->track_w * center_z;
	float track_y2 = track_y1 + track->track_h * center_z;

	output_to_canvas(mwindow->edl, 0, track_x1, track_y1);
	output_to_canvas(mwindow->edl, 0, track_x2, track_y2);

#define DRAW_PROJECTION(offset) \
	get_canvas()->draw_rectangle((int)track_x1 + offset, \
		(int)track_y1 + offset, \
		(int)(track_x2 - track_x1), \
		(int)(track_y2 - track_y1)); \
	get_canvas()->draw_line((int)track_x1 + offset,  \
		(int)track_y1 + offset, \
		(int)track_x2 + offset, \
		(int)track_y2 + offset); \
	get_canvas()->draw_line((int)track_x2 + offset,  \
		(int)track_y1 + offset, \
		(int)track_x1 + offset, \
		(int)track_y2 + offset); \

// Drop shadow
	get_canvas()->set_color(BLACK);
	DRAW_PROJECTION(1);

//	canvas->set_inverse();
	if(do_camera)
		get_canvas()->set_color(GREEN);
	else
		get_canvas()->set_color(RED);

	DRAW_PROJECTION(0);
}



int CWindowCanvas::test_bezier(int button_press, 
	int &redraw, 
	int &redraw_canvas,
	int &rerender,
	int do_camera)
{
	int result = 0;

// Processing drag operation.
// Create keyframe during first cursor motion.
	if(!button_press)
	{

		float cursor_x = get_cursor_x();
		float cursor_y = get_cursor_y();
		canvas_to_output(mwindow->edl, 0, cursor_x, cursor_y);

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
			float last_center_x;
			float last_center_y;
			float last_center_z;
			int created;

			if(!gui->affected_x && !gui->affected_y && !gui->affected_z)
			{
				FloatAutos *affected_x_autos;
				FloatAutos *affected_y_autos;
				FloatAutos *affected_z_autos;
				if(!gui->affected_track) return 0;
				ptstime position = mwindow->edl->local_session->get_selectionstart(1);
				posnum track_position = gui->affected_track->to_units(position, 0);

				if(mwindow->edl->session->cwindow_operation == CWINDOW_CAMERA)
				{
					affected_x_autos = (FloatAutos*)gui->affected_track->automation->autos[AUTOMATION_CAMERA_X];
					affected_y_autos = (FloatAutos*)gui->affected_track->automation->autos[AUTOMATION_CAMERA_Y];
					affected_z_autos = (FloatAutos*)gui->affected_track->automation->autos[AUTOMATION_CAMERA_Z];
				}
				else
				{
					affected_x_autos = (FloatAutos*)gui->affected_track->automation->autos[AUTOMATION_PROJECTOR_X];
					affected_y_autos = (FloatAutos*)gui->affected_track->automation->autos[AUTOMATION_PROJECTOR_Y];
					affected_z_autos = (FloatAutos*)gui->affected_track->automation->autos[AUTOMATION_PROJECTOR_Z];
				}


				if(gui->translating_zoom)
				{
					FloatAuto *previous = 0;
					FloatAuto *next = 0;
					float new_z = affected_z_autos->get_value(
						position,
						previous,
						next);
					gui->affected_z = 
						(FloatAuto*)gui->cwindow->calculate_affected_auto(
							affected_z_autos, 1, &created, 0);
					if(created) 
					{
						gui->affected_z->value = new_z;
						gui->affected_z->control_in_value = 0;
						gui->affected_z->control_out_value = 0;
						gui->affected_z->control_in_pts = 0;
						gui->affected_z->control_out_pts = 0;
						redraw_canvas = 1;
					}
				}
				else
				{
					FloatAuto *previous = 0;
					FloatAuto *next = 0;
					float new_x = affected_x_autos->get_value(
						position,
						previous,
						next);
					previous = 0;
					next = 0;
					float new_y = affected_y_autos->get_value(
						position, 
						previous,
						next);
					gui->affected_x = 
						(FloatAuto*)gui->cwindow->calculate_affected_auto(
							affected_x_autos, 1, &created, 0);
					if(created) 
					{
						gui->affected_x->value = new_x;
						gui->affected_x->control_in_value = 0;
						gui->affected_x->control_out_value = 0;
						gui->affected_x->control_in_pts = 0;
						gui->affected_x->control_out_pts = 0;
						redraw_canvas = 1;
					}
					gui->affected_y = 
						(FloatAuto*)gui->cwindow->calculate_affected_auto(
							affected_y_autos, 1, &created, 0);
					if(created) 
					{
						gui->affected_y->value = new_y;
						gui->affected_y->control_in_value = 0;
						gui->affected_y->control_out_value = 0;
						gui->affected_y->control_in_pts = 0;
						gui->affected_y->control_out_pts = 0;
						redraw_canvas = 1;
					}
				}

				calculate_origin();

				if(gui->translating_zoom)
				{
					gui->center_z = gui->affected_z->value;
				}
				else
				{
					gui->center_x = gui->affected_x->value;
					gui->center_y = gui->affected_y->value;
				}

				rerender = 1;
				redraw = 1;
			}


			if(gui->translating_zoom)
			{
				last_center_z = gui->affected_z->value;
			}
			else
			{
				last_center_x = gui->affected_x->value;
				last_center_y = gui->affected_y->value;
			}

			if(gui->translating_zoom)
			{
				gui->affected_z->value = gui->center_z + 
					(cursor_y - gui->y_origin) / 128;

				if(gui->affected_z->value < 0) gui->affected_z->value = 0;
				if(!EQUIV(last_center_z, gui->affected_z->value))
				{
					rerender = 1;
					redraw = 1;
					redraw_canvas = 1;
				}
			}
			else
			{
				gui->affected_x->value = gui->center_x + cursor_x - gui->x_origin;
				gui->affected_y->value = gui->center_y + cursor_y - gui->y_origin;
				if(!EQUIV(last_center_x,  gui->affected_x->value) ||
					!EQUIV(last_center_y, gui->affected_y->value))
				{
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
				mwindow->edl->session->cwindow_operation;
			result = 1;
		}
	}

	return result;
}

int CWindowCanvas::test_zoom(int &redraw)
{
	int result = 0;
	float zoom = get_zoom();
	float x;
	float y;

	if(!mwindow->edl->session->cwindow_scrollbars)
	{
		mwindow->edl->session->cwindow_scrollbars = 1;
		zoom = 1.0;
		x = mwindow->edl->session->output_w / 2;
		y = mwindow->edl->session->output_h / 2;
	}
	else
	{
		x = get_cursor_x();
		y = get_cursor_y();
		canvas_to_output(mwindow->edl, 
				0, 
				x, 
				y);

// Find current zoom in table
		int current_index = 0;
		for(current_index = 0 ; current_index < total_zooms; current_index++)
			if(EQUIV(my_zoom_table[current_index], zoom)) break;

// Zoom out
		if(get_buttonpress() == 5 ||
			gui->ctrl_down() || 
			gui->shift_down())
		{
			current_index--;
		}
		else
// Zoom in
		{
			current_index++;
		}
		CLAMP(current_index, 0, total_zooms - 1);
		zoom = my_zoom_table[current_index];
	}

	x = x - w / zoom / 2;
	y = y - h / zoom / 2;


	int x_i = (int)x;
	int y_i = (int)y;

	update_zoom(x_i, 
			y_i, 
			zoom);
	reposition_window(mwindow->edl, 
			mwindow->theme->ccanvas_x,
			mwindow->theme->ccanvas_y,
			mwindow->theme->ccanvas_w,
			mwindow->theme->ccanvas_h);
	redraw = 1;
	result = 1;

	gui->zoom_panel->update(zoom);
	
	return result;
}


void CWindowCanvas::calculate_origin()
{
	gui->x_origin = get_cursor_x();
	gui->y_origin = get_cursor_y();
	canvas_to_output(mwindow->edl, 0, gui->x_origin, gui->y_origin);
}


int CWindowCanvas::cursor_leave_event()
{
	set_cursor(ARROW_CURSOR);
	return 1;
}

int CWindowCanvas::cursor_enter_event()
{
	int redraw = 0;
	switch(mwindow->edl->session->cwindow_operation)
	{
		case CWINDOW_CAMERA:
		case CWINDOW_PROJECTOR:
			set_cursor(MOVE_CURSOR);
			break;
		case CWINDOW_ZOOM:
			set_cursor(MOVE_CURSOR);
			break;
		case CWINDOW_CROP:
			test_crop(0, redraw);
			break;
		case CWINDOW_PROTECT:
			set_cursor(ARROW_CURSOR);
			break;
		case CWINDOW_MASK:
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

	switch(gui->current_operation)
	{
	case CWINDOW_SCROLL:
		{
			float zoom = get_zoom();
			float cursor_x = get_cursor_x();
			float cursor_y = get_cursor_y();
			float zoom_x, zoom_y, conformed_w, conformed_h;

			get_zooms(mwindow->edl, 0, zoom_x, zoom_y, conformed_w, conformed_h);
			cursor_x = (float)cursor_x / zoom_x + gui->x_offset;
			cursor_y = (float)cursor_y / zoom_y + gui->y_offset;

			int x = (int)(gui->x_origin - cursor_x + gui->x_offset);
			int y = (int)(gui->y_origin - cursor_y + gui->y_offset);

			update_zoom(x, 
				y, 
				zoom);
			update_scrollbars();
			redraw = 1;
			result = 1;
			break;
		}

	case CWINDOW_CAMERA:
		result = test_bezier(0, redraw, redraw_canvas, rerender, 1);
		break;

	case CWINDOW_PROJECTOR:
		result = test_bezier(0, redraw, redraw_canvas, rerender, 0);
		break;

	case CWINDOW_CROP:
		result = test_crop(0, redraw);
		break;

	case CWINDOW_MASK:
	case CWINDOW_MASK_CONTROL_IN:
	case CWINDOW_MASK_CONTROL_OUT:
	case CWINDOW_MASK_TRANSLATE:
		result = do_mask(redraw, 
			rerender, 
			0, 
			1,
			0);
		break;

	case CWINDOW_EYEDROP:
		result = do_eyedrop(rerender, 0);
		break;
	}



	if(!result)
	{
		switch(mwindow->edl->session->cwindow_operation)
		{
		case CWINDOW_CROP:
			result = test_crop(0, redraw);
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
	{
		mwindow->gui->lock_window("CWindowCanvas::cursor_motion_event 1");
		mwindow->gui->canvas->draw_overlays();
		mwindow->gui->canvas->flash();
		mwindow->gui->unlock_window();
	}

	if(rerender)
	{
		mwindow->restart_brender();
		mwindow->sync_parameters(CHANGE_PARAMS);
		gui->cwindow->playback_engine->send_command(CURRENT_FRAME, mwindow->edl);
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

	if(Canvas::button_press_event()) return 1;

	gui->translating_zoom = gui->shift_down(); 

	calculate_origin();

	float zoom_x, zoom_y, conformed_w, conformed_h;
	get_zooms(mwindow->edl, 0, zoom_x, zoom_y, conformed_w, conformed_h);
	gui->x_offset = get_x_offset(mwindow->edl, 0, zoom_x, conformed_w, conformed_h);
	gui->y_offset = get_y_offset(mwindow->edl, 0, zoom_y, conformed_w, conformed_h);

// Scroll view
	if(get_buttonpress() == 2)
	{
		gui->current_operation = CWINDOW_SCROLL;
		result = 1;
	}
	else
// Adjust parameter
	{
		switch(mwindow->edl->session->cwindow_operation)
		{
		case CWINDOW_CAMERA:
			result = test_bezier(1, redraw, redraw_canvas, rerender, 1);
			break;

		case CWINDOW_PROJECTOR:
			result = test_bezier(1, redraw, redraw_canvas, rerender, 0);
			break;

		case CWINDOW_ZOOM:
			result = test_zoom(redraw);
			break;

		case CWINDOW_CROP:
			result = test_crop(1, redraw);
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
		mwindow->restart_brender();
		mwindow->sync_parameters(CHANGE_PARAMS);
		gui->cwindow->playback_engine->send_command(CURRENT_FRAME, mwindow->edl);
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

	case CWINDOW_CAMERA:
		mwindow->undo->update_undo(_("camera"), LOAD_AUTOMATION);
		break;

	case CWINDOW_PROJECTOR:
		mwindow->undo->update_undo(_("projector"), LOAD_AUTOMATION);
		break;

	case CWINDOW_MASK:
	case CWINDOW_MASK_CONTROL_IN:
	case CWINDOW_MASK_CONTROL_OUT:
	case CWINDOW_MASK_TRANSLATE:
		mwindow->undo->update_undo(_("mask point"), LOAD_AUTOMATION);
		break;

	}

	gui->current_operation = CWINDOW_NONE;
	return result;
}

void CWindowCanvas::zoom_resize_window(float percentage)
{
	int canvas_w, canvas_h;
	calculate_sizes(mwindow->edl->get_aspect_ratio(), 
		mwindow->edl->session->output_w, 
		mwindow->edl->session->output_h, 
		percentage,
		canvas_w,
		canvas_h);

	int new_w, new_h;
	new_w = canvas_w + (gui->get_w() - mwindow->theme->ccanvas_w);
	new_h = canvas_h + (gui->get_h() - mwindow->theme->ccanvas_h);
	gui->resize_window(new_w, new_h);
	gui->resize_event(new_w, new_h);
}

void CWindowCanvas::toggle_controls()
{
	mwindow->session->cwindow_controls = !mwindow->session->cwindow_controls;
	gui->resize_event(gui->get_w(), gui->get_h());
}

int CWindowCanvas::get_cwindow_controls()
{
	return mwindow->session->cwindow_controls;
}
