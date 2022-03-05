// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bcsignals.h"
#include "bcresources.h"
#include "canvas.h"
#include "clip.h"
#include "colors.h"
#include "cwindowgui.h"
#include "edl.h"
#include "edlsession.h"
#include "keys.h"
#include "language.h"
#include "mainsession.h"
#include "mutex.h"
#include "tmpframecache.h"
#include "vframe.h"
#include "vwindowgui.h"


Canvas::Canvas(CWindowGUI *cwindow,
	VWindowGUI *vwindow,
	int x, 
	int y, 
	int w, 
	int h,
	int use_scrollbars)
{
	xscroll = 0;
	yscroll = 0;
	refresh_frame = 0;
	canvas_subwindow = 0;
	canvas_fullscreen = 0;
	is_processing = 0;
	cursor_inside = 0;

	if(x < 10) x = 10;
	if(y < 10) y = 10;

	cwindowgui = cwindow;
	vwindowgui = vwindow;
	if(cwindow)
		subwindow = cwindow;
	else
		subwindow = vwindow;
	this->x = x;
	this->y = y;
	this->w = w;
	this->h = h;
	this->use_scrollbars = use_scrollbars;
	this->root_w = subwindow->get_root_w(0, 0);
	this->root_h = subwindow->get_root_h(0);
	canvas_lock = new Mutex("Canvas::canvas_lock", 1);
	guidelines.set_canvas(this);
	subwindow->add_subwindow(canvas_menu = new CanvasPopup(this));

	subwindow->add_subwindow(fullscreen_menu = new CanvasFullScreenPopup(this));
}

Canvas::~Canvas()
{
	BC_Resources::tmpframes.release_frame(refresh_frame);
	delete canvas_menu;
	if(yscroll) delete yscroll;
	if(xscroll) delete xscroll;
	delete canvas_subwindow;
	delete canvas_fullscreen;
	delete canvas_lock;
	canvas_lock = 0;
}

void Canvas::release_refresh_frame()
{
	if(refresh_frame)
	{
		lock_canvas("Canvas::release_refresh_frame");
		BC_Resources::tmpframes.release_frame(refresh_frame);
		refresh_frame = 0;
		unlock_canvas();
	}
}

void Canvas::lock_canvas(const char *location)
{
	if(canvas_lock)
		canvas_lock->lock(location);
}

void Canvas::unlock_canvas()
{
	if(canvas_lock)
		canvas_lock->unlock();
}

int Canvas::is_locked()
{
	return canvas_lock->is_locked();
}

BC_WindowBase* Canvas::get_canvas()
{
	if(!canvas_subwindow && !canvas_fullscreen)
	{
		view_x = x;
		view_y = y;
		view_w = w;
		view_h = h;
		get_scrollbars(master_edl, view_x, view_y, view_w, view_h);
		create_canvas();
	}
	if(get_fullscreen() && canvas_fullscreen) 
		return canvas_fullscreen;
	else
		return canvas_subwindow;
}

void Canvas::clear_canvas(int do_flash)
{
	BC_WindowBase *cur_canvas = get_canvas();

	cur_canvas->set_color(BLACK);
	cur_canvas->draw_box(0, 0, cur_canvas->get_w(), cur_canvas->get_h());
	if(do_flash)
		cur_canvas->flash();
}

// Get dimensions given a zoom
void Canvas::calculate_sizes(int output_w, int output_h,
	double zoom, int &w, int &h)
{
	double aspect = sample_aspect_ratio();
	// Horizontal stretch
	if(aspect >= 1)
	{
		w = round(output_w * aspect * zoom);
		h = round(output_h * zoom);
	}
	else
	// Vertical stretch
	{
		h = round(output_h * zoom);
		w = round(output_w / aspect * zoom);
	}
}

double Canvas::get_x_offset(EDL *edl,
	double zoom_x,
	double conformed_w,
	double conformed_h)
{
	if(use_scrollbars)
	{
		if(xscroll) 
		{
			return get_xscroll();
		}
		else
			return ((double)-get_canvas()->get_w() / zoom_x +
				edlsession->output_w) / 2;
	}
	else
	{
		int out_w, out_h;
		int canvas_w = get_canvas()->get_w();
		int canvas_h = get_canvas()->get_h();
		out_w = canvas_w;
		out_h = canvas_h;

		if((double)out_w / out_h > conformed_w / conformed_h)
		{
			out_w = round(out_h * conformed_w / conformed_h);
		}

		if(out_w < canvas_w)
			return -(canvas_w - out_w) / 2 / zoom_x;
	}
	return 0;
}

double Canvas::get_y_offset(EDL *edl,
	double zoom_y,
	double conformed_w,
	double conformed_h)
{
	if(use_scrollbars)
	{
		if(yscroll)
		{
			return get_yscroll();
		}
		else
			return ((double)-get_canvas()->get_h() / zoom_y + 
				edlsession->output_h) / 2;
	}
	else
	{
		int out_w, out_h;
		int canvas_w = get_canvas()->get_w();
		int canvas_h = get_canvas()->get_h();
		out_w = canvas_w;
		out_h = canvas_h;

		if((double)out_w / out_h <= conformed_w / conformed_h)
		{
			out_h = round(out_w / (conformed_w / conformed_h));
		}

		if(out_h < canvas_h)
			return -(canvas_h - out_h) / 2 / zoom_y;
	}

	return 0;
}

void Canvas::update_scrollbars()
{
	if(use_scrollbars)
	{
		if(xscroll) xscroll->update_length(w_needed, get_xscroll(), w_visible);
		if(yscroll) yscroll->update_length(h_needed, get_yscroll(), h_visible);
	}
}

void Canvas::get_zooms(EDL *edl,
	double &zoom_x,
	double &zoom_y,
	double &conformed_w,
	double &conformed_h)
{
	edl->calculate_conformed_dimensions(conformed_w, conformed_h);

	if(use_scrollbars)
	{
		zoom_x = get_zoom() * edl->get_sample_aspect_ratio();
		zoom_y = get_zoom();
	}
	else
	{
		double out_w = get_canvas()->get_w();
		double out_h = get_canvas()->get_h();

		if(out_w / out_h > conformed_w / conformed_h)
		{
			out_w = round(out_h * conformed_w / conformed_h);
		}
		else
		{
			out_h = round(out_w / (conformed_w / conformed_h));
		}
		zoom_x = out_w / edlsession->output_w;
		zoom_y = out_h / edlsession->output_h;
	}
}

// Convert a coordinate on the canvas to a coordinate on the output
void Canvas::canvas_to_output(EDL *edl, double &x, double &y)
{
	double zoom_x, zoom_y, conformed_w, conformed_h;
	get_zooms(edl, zoom_x, zoom_y, conformed_w, conformed_h);

	x = x / zoom_x + get_x_offset(edl, zoom_x, conformed_w, conformed_h);
	y = y / zoom_y + get_y_offset(edl, zoom_y, conformed_w, conformed_h);
}

void Canvas::output_to_canvas(EDL *edl, double &x, double &y)
{
	double zoom_x, zoom_y, conformed_w, conformed_h;
	get_zooms(edl, zoom_x, zoom_y, conformed_w, conformed_h);

	x = zoom_x * (x - get_x_offset(edl, zoom_x, conformed_w, conformed_h));
	y = zoom_y * (y - get_y_offset(edl, zoom_y, conformed_w, conformed_h));
}

void Canvas::get_transfers(EDL *edl, 
	double &output_x1,
	double &output_y1,
	double &output_x2,
	double &output_y2,
	double &canvas_x1,
	double &canvas_y1,
	double &canvas_x2,
	double &canvas_y2,
	int canvas_w,
	int canvas_h)
{
// automatic canvas size detection
	if(canvas_w < 0) canvas_w = get_canvas()->get_w();
	if(canvas_h < 0) canvas_h = get_canvas()->get_h();

// Canvas is zoomed to a portion of the output frame
	if(use_scrollbars)
	{
		double in_x1, in_y1, in_x2, in_y2;
		double out_x1, out_y1, out_x2, out_y2;
		double zoom_x, zoom_y, conformed_w, conformed_h;

		get_zooms(edl, zoom_x, zoom_y, conformed_w, conformed_h);
		out_x1 = 0;
		out_y1 = 0;
		out_x2 = canvas_w;
		out_y2 = canvas_h;
		in_x1 = 0;
		in_y1 = 0;
		in_x2 = canvas_w;
		in_y2 = canvas_h;

		canvas_to_output(edl, in_x1, in_y1);
		canvas_to_output(edl, in_x2, in_y2);

		if(in_x1 < 0)
		{
			out_x1 += -in_x1 * zoom_x;
			in_x1 = 0;
		}

		if(in_y1 < 0)
		{
			out_y1 += -in_y1 * zoom_y;
			in_y1 = 0;
		}

		int output_w = get_output_w(edl);
		int output_h = get_output_h(edl);

		if(in_x2 > output_w)
		{
			out_x2 -= (in_x2 - output_w) * zoom_x;
			in_x2 = output_w;
		}

		if(in_y2 > output_h)
		{
			out_y2 -= (in_y2 - output_h) * zoom_y;
			in_y2 = output_h;
		}

		output_x1 = in_x1;
		output_y1 = in_y1;
		output_x2 = in_x2;
		output_y2 = in_y2;
		canvas_x1 = out_x1;
		canvas_y1 = out_y1;
		canvas_x2 = out_x2;
		canvas_y2 = out_y2;
	}
	else
// The output frame is normalized to the canvas
	{
// Default canvas coords fill the entire canvas
		canvas_x1 = 0;
		canvas_y1 = 0;
		canvas_x2 = canvas_w;
		canvas_y2 = canvas_h;

		if(edl)
		{
// Use EDL aspect ratio to shrink one of the canvas dimensions
			double out_w = canvas_x2 - canvas_x1;
			double out_h = canvas_y2 - canvas_y1;
			double aspect = sample_aspect_ratio() *
				edlsession->output_w / edlsession->output_h;
			if(out_w / out_h > aspect)
			{
				out_w = round(out_h * aspect);
				canvas_x1 = canvas_w / 2 - out_w / 2;
			}
			else
			{
				out_h = round(out_w / aspect);
				canvas_y1 = canvas_h / 2 - out_h / 2;
			}
			canvas_x2 = canvas_x1 + out_w;
			canvas_y2 = canvas_y1 + out_h;

// Get output frame coords from EDL
			output_x1 = 0;
			output_y1 = 0;
			output_x2 = get_output_w(edl);
			output_y2 = get_output_h(edl);
		}
	}

// Clamp to minimum value
	output_x1 = MAX(0, output_x1);
	output_y1 = MAX(0, output_y1);
	output_x2 = MAX(output_x1, output_x2);
	output_y2 = MAX(output_y1, output_y2);
	canvas_x1 = MAX(0, canvas_x1);
	canvas_y1 = MAX(0, canvas_y1);
	canvas_x2 = MAX(canvas_x1, canvas_x2);
	canvas_y2 = MAX(canvas_y1, canvas_y2);
}

int Canvas::scrollbars_exist()
{
	return(use_scrollbars && (xscroll || yscroll));
}

int Canvas::get_output_w(EDL *edl)
{
	if(edl->this_edlsession)
		return edl->this_edlsession->output_w;
	return edlsession->output_w;
}

int Canvas::get_output_h(EDL *edl)
{
	if(edl->this_edlsession)
		return edl->this_edlsession->output_h;
	return edlsession->output_h;
}

void Canvas::get_scrollbars(EDL *edl, 
	int &canvas_x, 
	int &canvas_y, 
	int &canvas_w, 
	int &canvas_h)
{
	int need_xscroll = 0;
	int need_yscroll = 0;
	double zoom_x, zoom_y, conformed_w, conformed_h;

	if(edl)
	{
		w_needed = edlsession->output_w;
		h_needed = edlsession->output_h;
		w_visible = w_needed;
		h_visible = h_needed;

		if(use_scrollbars)
		{
			get_zooms(edl, zoom_x, zoom_y, conformed_w, conformed_h);

			if(!need_xscroll)
			{
				need_xscroll = 1;
				canvas_h -= BC_ScrollBar::get_span(SCROLL_HORIZ);
			}

			if(!need_yscroll)
			{
				need_yscroll = 1;
				canvas_w -= BC_ScrollBar::get_span(SCROLL_VERT);
			}

			w_visible = round(canvas_w / zoom_x);
			h_visible = round(canvas_h / zoom_y);
		}
		if(need_xscroll)
		{
			if(!xscroll)
				subwindow->add_subwindow(xscroll = new CanvasXScroll(edl,
					this,
					canvas_x,
					canvas_y + canvas_h,
					w_needed,
					get_xscroll(),
					w_visible,
					canvas_w));
			else
				xscroll->reposition_window(canvas_x, canvas_y + canvas_h, canvas_w);

			if(xscroll->get_length() != w_needed ||
					xscroll->get_handlelength() != w_visible)
				xscroll->update_length(w_needed, get_xscroll(), w_visible);
		}
		else
		{
			if(xscroll) delete xscroll;
			xscroll = 0;
		}
		if(need_yscroll)
		{
			if(!yscroll)
				subwindow->add_subwindow(yscroll = new CanvasYScroll(edl,
					this,
					canvas_x + canvas_w,
					canvas_y,
					h_needed,
					get_yscroll(),
					h_visible,
					canvas_h));
			else
				yscroll->reposition_window(canvas_x + canvas_w, canvas_y, canvas_h);

			if(yscroll->get_length() != edlsession->output_h ||
					yscroll->get_handlelength() != h_visible)
				yscroll->update_length(h_needed, get_yscroll(), h_visible);
		}
		else
		{
			if(yscroll) delete yscroll;
			yscroll = 0;
		}
	}
}

void Canvas::reposition_window(EDL *edl, int x, int y, int w, int h)
{
	this->x = x;
	this->y = y;
	this->w = w;
	this->h = h;
	view_x = x;
	view_y = y;
	view_w = w;
	view_h = h;
	get_scrollbars(edl, view_x, view_y, view_w, view_h);
	if(canvas_subwindow)
	{
		canvas_subwindow->reposition_window(view_x, view_y, view_w, view_h);
		clear_canvas();
	}

	draw_refresh();
}

void Canvas::set_cursor(int cursor)
{
	get_canvas()->set_cursor(cursor);
}

int Canvas::get_cursor_x()
{
	return get_canvas()->get_cursor_x();
}

int Canvas::get_cursor_y()
{
	return get_canvas()->get_cursor_y();
}

int Canvas::get_buttonpress()
{
	return get_canvas()->get_buttonpress();
}

int Canvas::button_press_event()
{
	int result = 0;

	if(!canvas_subwindow->get_video_on() && get_canvas()->get_buttonpress() == 3)
	{
		if(get_fullscreen())
			fullscreen_menu->activate_menu();
		else
			canvas_menu->activate_menu();
		result = 1;
	}

	return result;
}

void Canvas::start_single()
{
	is_processing = 1;
	status_event();
}

void Canvas::stop_single()
{
	is_processing = 0;
	status_event();
}

void Canvas::start_video()
{
	if(get_canvas())
	{
		get_canvas()->start_video();
		status_event();
	}
}

void Canvas::stop_video()
{
	if(get_canvas())
	{
		get_canvas()->stop_video();
		status_event();
	}
}

void Canvas::start_fullscreen()
{
	zoom_auto();
	set_fullscreen(1);
	create_canvas();
}

void Canvas::stop_fullscreen()
{
	set_fullscreen(0);
	create_canvas();
}

void Canvas::create_canvas()
{
	int video_on = 0;

	lock_canvas("Canvas::create_canvas");

	if(!get_fullscreen())
	{
		if(canvas_fullscreen)
		{
			video_on = canvas_fullscreen->get_video_on();
			canvas_fullscreen->stop_video();
			canvas_fullscreen->hide_window();
		}

		if(!canvas_subwindow)
		{
			subwindow->add_subwindow(canvas_subwindow = new CanvasOutput(this, 
				view_x, 
				view_y, 
				view_w, 
				view_h));
		}
	}
	else
	{
		if(canvas_subwindow)
		{
			video_on = canvas_subwindow->get_video_on();
			canvas_subwindow->stop_video();
		}

		if(!canvas_fullscreen)
		{
			canvas_fullscreen = new CanvasFullScreen(this,
							root_w,
							root_h);
		}
		else
		{
			canvas_fullscreen->show_window();
		}
	}

	if(!video_on)
	{
		clear_canvas(0);
		draw_refresh();
	}
	else
		get_canvas()->start_video();
	unlock_canvas();
}

int Canvas::cursor_leave_event_base(BC_WindowBase *caller)
{
	int result = 0;
	if(cursor_inside) result = cursor_leave_event();
	cursor_inside = 0;
	return result;
}

int Canvas::cursor_enter_event_base(BC_WindowBase *caller)
{
	int result = 0;
	if(caller->is_event_win() && caller->cursor_inside())
	{
		cursor_inside = 1;
		result = cursor_enter_event();
	}
	return result;
}

int Canvas::button_press_event_base(BC_WindowBase *caller)
{
	if(caller->is_event_win() && caller->cursor_inside())
	{
		return button_press_event();
	}
	return 0;
}

int Canvas::keypress_event(BC_WindowBase *caller)
{
	if(caller->get_keypress() == 'f')
	{
		if(get_fullscreen())
			stop_fullscreen();
		else
			start_fullscreen();
		return 1;
	}
	else
	if(caller->get_keypress() == ESC)
	{
		if(get_fullscreen())
			stop_fullscreen();
		return 1;
	}
	return 0;
}


CanvasOutput::CanvasOutput(Canvas *canvas,
    int x,
    int y,
    int w,
    int h)
 : BC_SubWindow(x, y, w, h, BLACK)
{
	this->canvas = canvas;
}

void CanvasOutput::cursor_leave_event()
{
	canvas->cursor_leave_event_base(this);
}

int CanvasOutput::cursor_enter_event()
{
	return canvas->cursor_enter_event_base(this);
}

int CanvasOutput::button_press_event()
{
	return canvas->button_press_event_base(this);
}

int CanvasOutput::button_release_event()
{
	return canvas->button_release_event();
}

int CanvasOutput::cursor_motion_event()
{
	return canvas->cursor_motion_event();
}

int CanvasOutput::keypress_event()
{
	return canvas->keypress_event(this);
}

void CanvasOutput::repeat_event(int duration)
{
	canvas->guidelines.repeat_event(duration);
}


CanvasFullScreen::CanvasFullScreen(Canvas *canvas,
	int w,
	int h)
 : BC_FullScreen(canvas->subwindow, 
	w, 
	h, 
	BLACK,
	0,
	0,
	0)
{
	this->canvas = canvas;
}


CanvasXScroll::CanvasXScroll(EDL *edl, 
	Canvas *canvas, 
	int x,
	int y,
	int length, 
	int position, 
	int handle_length,
	int pixels)
 : BC_ScrollBar(x, 
		y, 
		SCROLL_HORIZ, 
		pixels, 
		length, 
		position, 
		handle_length)
{
	this->canvas = canvas;
}

int CanvasXScroll::handle_event()
{
	canvas->update_zoom(get_value(), canvas->get_yscroll(), canvas->get_zoom());
	canvas->draw_refresh();
	return 1;
}


CanvasYScroll::CanvasYScroll(EDL *edl, 
	Canvas *canvas, 
	int x,
	int y,
	int length, 
	int position, 
	int handle_length,
	int pixels)
 : BC_ScrollBar(x, 
		y, 
		SCROLL_VERT, 
		pixels, 
		length, 
		position, 
		handle_length)
{
	this->canvas = canvas;
}

int CanvasYScroll::handle_event()
{
	canvas->update_zoom(canvas->get_xscroll(), get_value(), canvas->get_zoom());
	canvas->draw_refresh();
	return 1;
}


CanvasFullScreenPopup::CanvasFullScreenPopup(Canvas *canvas)
 : BC_PopupMenu(0, 
		0, 
		0, 
		"", 
		0)
{
	add_item(new CanvasSubWindowItem(canvas));
}


CanvasSubWindowItem::CanvasSubWindowItem(Canvas *canvas)
 : BC_MenuItem(_("Windowed"), "f", 'f')
{
	this->canvas = canvas;
}

int CanvasSubWindowItem::handle_event()
{
// It isn't a problem to delete the canvas from in here because the event
// dispatcher is the canvas subwindow.
	canvas->stop_fullscreen();
	return 1;
}


CanvasPopup::CanvasPopup(Canvas *canvas)
 : BC_PopupMenu(0, 
		0, 
		0, 
		"", 
		0)
{
	add_item(new CanvasPopupSize(canvas, _("Zoom 25%"), 0.25));
	add_item(new CanvasPopupSize(canvas, _("Zoom 33%"), 0.33));
	add_item(new CanvasPopupSize(canvas, _("Zoom 50%"), 0.5));
	add_item(new CanvasPopupSize(canvas, _("Zoom 75%"), 0.75));
	add_item(new CanvasPopupSize(canvas, _("Zoom 100%"), 1.0));
	add_item(new CanvasPopupSize(canvas, _("Zoom 150%"), 1.5));
	add_item(new CanvasPopupSize(canvas, _("Zoom 200%"), 2.0));
	add_item(new CanvasPopupSize(canvas, _("Zoom 300%"), 3.0));
	add_item(new CanvasPopupSize(canvas, _("Zoom 400%"), 4.0));
	if(canvas->cwindowgui)
	{
		add_item(new CanvasPopupAuto(canvas));
		add_item(new CanvasPopupResetCamera(canvas));
		add_item(new CanvasPopupResetProjector(canvas));
		add_item(new CanvasToggleControls(canvas));
	}
	if(canvas->vwindowgui)
	{
		add_item(new CanvasPopupRemoveSource(canvas));
	}
	add_item(new CanvasFullScreenItem(canvas));
}


CanvasPopupAuto::CanvasPopupAuto(Canvas *canvas)
 : BC_MenuItem(_("Zoom Auto"))
{
	this->canvas = canvas;
}

int CanvasPopupAuto::handle_event()
{
	canvas->zoom_auto();
	return 1;
}


CanvasPopupSize::CanvasPopupSize(Canvas *canvas, char *text, double percentage)
 : BC_MenuItem(text)
{
	this->canvas = canvas;
	this->percentage = percentage;
}

int CanvasPopupSize::handle_event()
{
	canvas->zoom_resize_window(percentage);
	return 1;
}


CanvasPopupResetCamera::CanvasPopupResetCamera(Canvas *canvas)
 : BC_MenuItem(_("Reset camera"))
{
	this->canvas = canvas;
}

int CanvasPopupResetCamera::handle_event()
{
	canvas->reset_camera();
	return 1;
}


CanvasPopupResetProjector::CanvasPopupResetProjector(Canvas *canvas)
 : BC_MenuItem(_("Reset projector"))
{
	this->canvas = canvas;
}

int CanvasPopupResetProjector::handle_event()
{
	canvas->reset_projector();
	return 1;
}


CanvasToggleControls::CanvasToggleControls(Canvas *canvas)
 : BC_MenuItem(calculate_text())
{
	this->canvas = canvas;
}
int CanvasToggleControls::handle_event()
{
	canvas->toggle_controls();
	set_text(calculate_text());
	return 1;
}

const char* CanvasToggleControls::calculate_text()
{
	if(!mainsession->cwindow_controls)
		return _("Show controls");
	else
		return _("Hide controls");
}


CanvasFullScreenItem::CanvasFullScreenItem(Canvas *canvas)
 : BC_MenuItem(_("Fullscreen"), "f", 'f')
{
	this->canvas = canvas;
}

int CanvasFullScreenItem::handle_event()
{
	canvas->start_fullscreen();
	return 1;
}


CanvasPopupRemoveSource::CanvasPopupRemoveSource(Canvas *canvas)
 : BC_MenuItem(_("Close source"))
{
	this->canvas = canvas;
}

int CanvasPopupRemoveSource::handle_event()
{
	canvas->close_source();
	return 1;
}
