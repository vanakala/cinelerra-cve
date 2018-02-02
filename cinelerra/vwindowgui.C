
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

#include "asset.h"
#include "assets.h"
#include "awindowgui.h"
#include "awindow.h"
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
#include "mwindowgui.h"
#include "mwindow.h"
#include "playtransport.h"
#include "preferences.h"
#include "theme.h"
#include "timebar.h"
#include "tracks.h"
#include "vframe.h"
#include "vplayback.h"
#include "vtimebar.h"
#include "vwindowgui.h"
#include "vwindow.h"


VWindowGUI::VWindowGUI(MWindow *mwindow, VWindow *vwindow)
 : BC_Window(MWindow::create_title(N_("Viewer")),
	mwindow->session->vwindow_x,
	mwindow->session->vwindow_y,
	mwindow->session->vwindow_w,
	mwindow->session->vwindow_h,
	100,
	100,
	1,
	1,
	1)
{
	this->mwindow = mwindow;
	this->vwindow = vwindow;
	loaded_title[0] = 0;

	set_icon(mwindow->theme->get_image("vwindow_icon"));

	mwindow->theme->get_vwindow_sizes(this);
	mwindow->theme->draw_vwindow_bg(this);
	flash();

	meters = new VWindowMeters(mwindow, 
		this,
		mwindow->theme->vmeter_x,
		mwindow->theme->vmeter_y,
		mwindow->theme->vmeter_h);

// Requires meters to build
	edit_panel = new VWindowEditing(mwindow, vwindow, meters, this);

	add_subwindow(slider = new VWindowSlider(mwindow, 
			vwindow,
			this,
			mwindow->theme->vslider_x,
			mwindow->theme->vslider_y,
			mwindow->theme->vslider_w));

	transport = new VWindowTransport(mwindow, 
		this, 
		mwindow->theme->vtransport_x, 
		mwindow->theme->vtransport_y);

	add_subwindow(clock = new MainClock(mwindow,
		mwindow->theme->vtime_x, 
		mwindow->theme->vtime_y,
		mwindow->theme->vtime_w));

	canvas = new VWindowCanvas(mwindow, this);

	add_subwindow(timebar = new VTimeBar(mwindow, 
		this,
		mwindow->theme->vtimebar_x,
		mwindow->theme->vtimebar_y,
		mwindow->theme->vtimebar_w, 
		mwindow->theme->vtimebar_h));
	timebar->update();

	deactivate();
	slider->activate();
	resize_event(mwindow->session->vwindow_w, mwindow->session->vwindow_h);
}

VWindowGUI::~VWindowGUI()
{
	delete canvas;
	delete transport;
}

void VWindowGUI::change_source(EDL *edl, const char *title)
{
	char string[BCTEXTLEN];

	if(title && title[0])
	{
		sprintf(string,"%s - " PROGRAM_NAME, title);
		strcpy(loaded_title, title);
	}
	else
	{
		strcpy(string, MWindow::create_title(N_("Viewer")));
		loaded_title[0] = 0;
	}
	slider->set_position();
	timebar->update();
	set_title(string);
}

void VWindowGUI::resize_event(int w, int h)
{
	mwindow->session->vwindow_x = get_x();
	mwindow->session->vwindow_y = get_y();
	mwindow->session->vwindow_w = w;
	mwindow->session->vwindow_h = h;

	mwindow->theme->get_vwindow_sizes(this);
	mwindow->theme->draw_vwindow_bg(this);
	flash();

	edit_panel->reposition_buttons(mwindow->theme->vedit_x, 
		mwindow->theme->vedit_y);
	slider->reposition_window(mwindow->theme->vslider_x, 
		mwindow->theme->vslider_y,
		mwindow->theme->vslider_w);
// Recalibrate pointer motion range
	slider->set_position();
	timebar->resize_event();
	transport->reposition_buttons(mwindow->theme->vtransport_x, 
		mwindow->theme->vtransport_y);
	clock->reposition_window(mwindow->theme->vtime_x, 
		mwindow->theme->vtime_y,
		mwindow->theme->vtime_w);
	canvas->reposition_window(0,
		mwindow->theme->vcanvas_x, 
		mwindow->theme->vcanvas_y, 
		mwindow->theme->vcanvas_w, 
		mwindow->theme->vcanvas_h);
	meters->reposition_window(mwindow->theme->vmeter_x,
		mwindow->theme->vmeter_y,
		mwindow->theme->vmeter_h);

	BC_WindowBase::resize_event(w, h);
}

void VWindowGUI::translation_event()
{
	mwindow->session->vwindow_x = get_x();
	mwindow->session->vwindow_y = get_y();
}

void VWindowGUI::close_event()
{
	vwindow->playback_engine->send_command(STOP);
	hide_window();
	mwindow->session->show_vwindow = 0;

	mwindow->gui->mainmenu->show_vwindow->set_checked(0);

	mwindow->save_defaults();
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
		mwindow->undo_entry();
		break;
	case 'Z':
		mwindow->redo_entry();
		break;
	case 'f':
		if(mwindow->session->vwindow_fullscreen)
			canvas->stop_fullscreen();
		else
			canvas->start_fullscreen();
		break;
	case ESC:
		if(mwindow->session->vwindow_fullscreen)
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
	if(get_hidden()) return;
	if(mwindow->session->current_operation != DRAG_ASSET) return;

	int old_status = mwindow->session->vcanvas_highlighted;

	int cursor_x = get_relative_cursor_x();
	int cursor_y = get_relative_cursor_y();

	mwindow->session->vcanvas_highlighted = (get_cursor_over_window() &&
		cursor_x >= canvas->x &&
		cursor_x < canvas->x + canvas->w &&
		cursor_y >= canvas->y &&
		cursor_y < canvas->y + canvas->h);

	if(old_status != mwindow->session->vcanvas_highlighted)
		canvas->draw_refresh();
}

int VWindowGUI::drag_stop()
{
	if(get_hidden()) return 0;

	if(mwindow->session->vcanvas_highlighted &&
		mwindow->session->current_operation == DRAG_ASSET)
	{
		mwindow->session->vcanvas_highlighted = 0;
		canvas->draw_refresh();

		Asset *asset = mwindow->session->drag_assets->total ?
			mwindow->session->drag_assets->values[0] : 0;
		EDL *edl = mwindow->session->drag_clips->total ?
			mwindow->session->drag_clips->values[0] : 0;

		if(asset)
			vwindow->change_source(asset);
		else
		if(edl)
			vwindow->change_source(edl);
		return 1;
	}

	return 0;
}


VWindowMeters::VWindowMeters(MWindow *mwindow, 
	VWindowGUI *gui, 
	int x, 
	int y, 
	int h)
 : MeterPanel(mwindow, 
		gui,
		x,
		y,
		h,
		mwindow->edl->session->audio_channels,
		mwindow->edl->session->vwindow_meter)
{
	this->mwindow = mwindow;
	this->gui = gui;
}

int VWindowMeters::change_status_event()
{
	mwindow->edl->session->vwindow_meter = use_meters;
	mwindow->theme->get_vwindow_sizes(gui);
	gui->resize_event(gui->get_w(), gui->get_h());
	return 1;
}


VWindowEditing::VWindowEditing(MWindow *mwindow, VWindow *vwindow, 
	MeterPanel *meter_panel, VWindowGUI *gui)
 : EditPanel(mwindow, 
		gui,
		mwindow->theme->vedit_x, 
		mwindow->theme->vedit_y,
		EDTP_SPLICE | EDTP_OVERWRITE | EDTP_COPY | EDTP_LABELS | EDTP_TOCLIP,
		meter_panel)
{
	this->mwindow = mwindow;
	this->vwindow = vwindow;
}

void VWindowEditing::copy_selection()
{
	vwindow->copy();
}

void VWindowEditing::splice_selection()
{
	if(vwindow->get_edl())
		mwindow->splice(vwindow->get_edl());
}

void VWindowEditing::overwrite_selection()
{
	if(vwindow->get_edl())
		mwindow->overwrite(vwindow->get_edl());
}

void VWindowEditing::toggle_label()
{
	if(vwindow->get_edl())
	{
		EDL *edl = vwindow->get_edl();
		edl->labels->toggle_label(edl->local_session->get_selectionstart(1),
			edl->local_session->get_selectionend(1));
		vwindow->gui->timebar->update();
	}
}

void VWindowEditing::prev_label()
{
	if(vwindow->get_edl())
	{
		EDL *edl = vwindow->get_edl();
		vwindow->playback_engine->interrupt_playback(1);

		Label *current = edl->labels->prev_label(
			edl->local_session->get_selectionstart(1));

		if(!current)
		{
			edl->local_session->set_selection(0);
			vwindow->update_position(CHANGE_NONE, 0, 1);
		}
		else
		{
			edl->local_session->set_selection(current->position);
			vwindow->update_position(CHANGE_NONE, 0, 1);
		}
	}
}

void VWindowEditing::next_label()
{
	if(vwindow->get_edl())
	{
		EDL *edl = vwindow->get_edl();
		Label *current = edl->labels->next_label(
			edl->local_session->get_selectionstart(1));
		if(!current)
		{
			vwindow->playback_engine->interrupt_playback(1);

			ptstime position = edl->tracks->total_length();
			edl->local_session->set_selection(position);
			vwindow->update_position(CHANGE_NONE, 0, 1);
		}
		else
		{
			vwindow->playback_engine->interrupt_playback(1);

			edl->local_session->set_selection(current->position);
			vwindow->update_position(CHANGE_NONE, 0, 1);
		}
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

void VWindowEditing::clear_inpoint()
{
	vwindow->clear_inpoint();
}

void VWindowEditing::clear_outpoint()
{
	vwindow->clear_outpoint();
}

void VWindowEditing::to_clip()
{
	if(vwindow->get_edl())
	{
		FileXML file;
		EDL *edl = vwindow->get_edl();
		ptstime start = edl->local_session->get_selectionstart();
		ptstime end = edl->local_session->get_selectionend();

		if(PTSEQU(start, end))
		{
			end = edl->tracks->total_length();
			start = 0;
		}

		edl->copy(start, 
			end, 
			1,
			0,
			0,
			&file,
			mwindow->plugindb,
			"",
			1);

		EDL *new_edl = new EDL(mwindow->edl);
		new_edl->load_xml(mwindow->plugindb, &file, LOAD_ALL);
		sprintf(new_edl->local_session->clip_title, _("Clip %d"), mwindow->session->clip_number++);
		char string[BCTEXTLEN];
		Units::totext(string, 
				end - start, 
				edl->session->time_format, 
				edl->session->sample_rate, 
				edl->session->frame_rate,
				edl->session->frames_per_foot);

		sprintf(new_edl->local_session->clip_notes, _("%s\n Created from:\n%s"), string, vwindow->gui->loaded_title);

		new_edl->local_session->set_selection(0);
		vwindow->clip_edit->create_clip(new_edl);
	}
}


VWindowSlider::VWindowSlider(MWindow *mwindow, 
	VWindow *vwindow, 
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
	this->mwindow = mwindow;
	this->vwindow = vwindow;
	this->gui = gui;
	set_precision(0.00001);
	set_pagination(1.0, 10.0);
}

int VWindowSlider::handle_event()
{
	vwindow->playback_engine->interrupt_playback(1);

	vwindow->update_position(CHANGE_NONE, 1, 0);
	return 1;
}

void VWindowSlider::set_position()
{
	EDL *edl = vwindow->get_edl();
	if(edl)
	{
		ptstime new_length = edl->tracks->total_playable_length();
		if(EQUIV(edl->local_session->preview_end, 0))
			edl->local_session->preview_end = new_length;
		if(edl->local_session->preview_end > new_length)
			edl->local_session->preview_end = new_length;
		if(edl->local_session->preview_start > new_length)
			edl->local_session->preview_start = 0;

		update(mwindow->theme->vslider_w, 
			edl->local_session->get_selectionstart(1), 
			edl->local_session->preview_start, 
			edl->local_session->preview_end);
	}
	else
		update(0, 0, 0, 0);
}


VWindowTransport::VWindowTransport(MWindow *mwindow, 
	VWindowGUI *gui, 
	int x, 
	int y)
 : PlayTransport(mwindow, 
	gui, 
	x, 
	y)
{
	this->gui = gui;
}

EDL* VWindowTransport::get_edl()
{
	return gui->vwindow->get_edl();
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


VWindowCanvas::VWindowCanvas(MWindow *mwindow, VWindowGUI *gui)
 : Canvas(mwindow,
	gui,
	mwindow->theme->vcanvas_x, 
	mwindow->theme->vcanvas_y, 
	mwindow->theme->vcanvas_w, 
	mwindow->theme->vcanvas_h,
	0,
	0,
	0,
	0,
	0,
	1)
{
	this->mwindow = mwindow;
	this->gui = gui;
}

void VWindowCanvas::zoom_resize_window(double percentage)
{
	EDL *edl = mwindow->vwindow->get_edl();
	if(!edl) edl = mwindow->edl;

	int canvas_w, canvas_h;
	calculate_sizes(edl,
		edl->session->output_w, 
		edl->session->output_h, 
		percentage,
		canvas_w,
		canvas_h);
	int new_w, new_h;
	new_w = canvas_w + (gui->get_w() - mwindow->theme->vcanvas_w);
	new_h = canvas_h + (gui->get_h() - mwindow->theme->vcanvas_h);
	gui->resize_window(new_w, new_h);
	gui->resize_event(new_w, new_h);
}

void VWindowCanvas::close_source()
{
	gui->vwindow->playback_engine->interrupt_playback(1);
	gui->vwindow->remove_source();
}

void VWindowCanvas::draw_refresh()
{
	EDL *edl = gui->vwindow->get_edl();
	if(!get_canvas()->get_video_on())
	{
		lock_canvas("VWindowCanvas::draw_refresh");
		get_canvas()->clear_box(0, 0, get_canvas()->get_w(), get_canvas()->get_h());
		if(refresh_frame && edl)
		{
			double in_x1, in_y1, in_x2, in_y2;
			double out_x1, out_y1, out_x2, out_y2;
			get_transfers(edl,
				in_x1,
				in_y1,
				in_x2,
				in_y2,
				out_x1,
				out_y1,
				out_x2,
				out_y2);
			get_canvas()->draw_vframe(refresh_frame,
				round(out_x1),
				round(out_y1),
				round(out_x2 - out_x1),
				round(out_y2 - out_y1),
				round(in_x1),
				round(in_y1),
				round(in_x2 - in_x1),
				round(in_y2 - in_y1),
				0);
		}
		unlock_canvas();
		draw_overlays();
		get_canvas()->flash();
	}
}

void VWindowCanvas::draw_overlays()
{
	if(mwindow->session->vcanvas_highlighted)
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
	return mwindow->session->vwindow_fullscreen;
}

void VWindowCanvas::set_fullscreen(int value)
{
	mwindow->session->vwindow_fullscreen = value;
}
