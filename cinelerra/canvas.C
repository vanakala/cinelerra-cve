#include "canvas.h"
#include "clip.h"
#include "edl.h"
#include "edlsession.h"
#include "vframe.h"


Canvas::Canvas(BC_WindowBase *subwindow, 
	int x, 
	int y, 
	int w, 
	int h,
	int output_w,
	int output_h,
	int use_scrollbars,
	int use_cwindow,
	int use_rwindow,
	int use_vwindow)
{
	reset();
	this->subwindow = subwindow;
	this->x = x;
	this->y = y;
	this->w = w;
	this->h = h;
	this->output_w = output_w;
	this->output_h = output_h;
	this->use_scrollbars = use_scrollbars;
	this->use_cwindow = use_cwindow;
	this->use_rwindow = use_rwindow;
	this->use_vwindow = use_vwindow;
}

Canvas::~Canvas()
{
	if(refresh_frame) delete refresh_frame;
	delete canvas_menu;
 	if(yscroll) delete yscroll;
 	if(xscroll) delete xscroll;
}

void Canvas::reset()
{
	use_scrollbars = 0;
	output_w = 0;
	output_h = 0;
    xscroll = 0;
    yscroll = 0;
	refresh_frame = 0;
}

// Get dimensions given a zoom
void Canvas::calculate_sizes(float aspect_ratio, 
	int output_w, 
	int output_h, 
	float zoom, 
	int &w, 
	int &h)
{
// Horizontal stretch
	if((float)output_w / output_h <= aspect_ratio)
	{
		w = (int)((float)output_h * aspect_ratio * zoom + 1);
		h = (int)((float)output_h * zoom + 1);
	}
	else
// Vertical stretch
	{
		h = (int)((float)output_w / aspect_ratio * zoom + 1);
		w = (int)((float)output_w * zoom + 1);
	}
}

float Canvas::get_x_offset(EDL *edl, 
	int single_channel, 
	float zoom_x, 
	float conformed_w,
	float conformed_h)
{
	if(use_scrollbars)
	{
		if(xscroll) 
		{
			if(conformed_w < w_visible)
				return -(float)(w_visible - conformed_w) / 2;

			return (float)get_xscroll();
		}
		else
			return ((float)-canvas->get_w() / zoom_x + 
				edl->calculate_output_w(single_channel)) / 2;
	}
	else
	{
		int out_w, out_h;
		int canvas_w = canvas->get_w();
		int canvas_h = canvas->get_h();
		out_w = canvas_w;
		out_h = canvas_h;
		
		if((float)out_w / out_h > conformed_w / conformed_h)
		{
			out_w = (int)(out_h * conformed_w / conformed_h + 0.5);
		}
		
		if(out_w < canvas_w)
			return -(canvas_w - out_w) / 2 / zoom_x;
	}

	return 0;
}

float Canvas::get_y_offset(EDL *edl, 
	int single_channel, 
	float zoom_y, 
	float conformed_w,
	float conformed_h)
{
	if(use_scrollbars)
	{
		if(yscroll)
		{
			if(conformed_h < h_visible)
				return -(float)(h_visible - conformed_h) / 2;

			return (float)get_yscroll();
		}
		else
			return ((float)-canvas->get_h() / zoom_y + 
				edl->calculate_output_h(single_channel)) / 2;
	}
	else
	{
		int out_w, out_h;
		int canvas_w = canvas->get_w();
		int canvas_h = canvas->get_h();
		out_w = canvas_w;
		out_h = canvas_h;

		if((float)out_w / out_h <= conformed_w / conformed_h)
		{
			out_h = (int)((float)out_w / (conformed_w / conformed_h) + 0.5);
		}

//printf("Canvas::get_y_offset 1 %d %d %f\n", out_h, canvas_h, -((float)canvas_h - out_h) / 2);
		if(out_h < canvas_h)
			return -((float)canvas_h - out_h) / 2 / zoom_y;
	}

	return 0;
}

// This may not be used anymore
void Canvas::check_boundaries(EDL *edl, int &x, int &y, float &zoom)
{
	if(x + w_visible > w_needed) x = w_needed - w_visible;
	if(y + h_visible > h_needed) y = h_needed - h_visible;

	if(x < 0) x = 0;
	if(y < 0) y = 0;
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
	int single_channel, 
	float &zoom_x, 
	float &zoom_y,
	float &conformed_w,
	float &conformed_h)
{
	edl->calculate_conformed_dimensions(single_channel, 
		conformed_w, 
		conformed_h);

	if(use_scrollbars)
	{
		zoom_x = get_zoom() * 
			conformed_w / 
			edl->calculate_output_w(single_channel);
		zoom_y = get_zoom() * 
			conformed_h / 
			edl->calculate_output_h(single_channel);
	}
	else
	{
		int out_w, out_h;
		int canvas_w = canvas->get_w();
		int canvas_h = canvas->get_h();
	
		out_w = canvas_w;
		out_h = canvas_h;

		if((float)out_w / out_h > conformed_w / conformed_h)
		{
			out_w = (int)((float)out_h * conformed_w / conformed_h + 0.5);
		}
		else
		{
			out_h = (int)((float)out_w / (conformed_w / conformed_h) + 0.5);
		}

		zoom_x = (float)out_w / edl->calculate_output_w(single_channel);
		zoom_y = (float)out_h / edl->calculate_output_h(single_channel);
//printf("get zooms 2 %d %d %f %f\n", canvas_w, canvas_h, conformed_w, conformed_h);
	}
}

// Convert a coordinate on the canvas to a coordinate on the output
void Canvas::canvas_to_output(EDL *edl, int single_channel, float &x, float &y)
{
	float zoom_x, zoom_y, conformed_w, conformed_h;
	get_zooms(edl, single_channel, zoom_x, zoom_y, conformed_w, conformed_h);

//printf("Canvas::canvas_to_output y=%f zoom_y=%f y_offset=%f\n", 
//	y, zoom_y, get_y_offset(edl, single_channel, zoom_y, conformed_w, conformed_h));

	x = (float)x / zoom_x + get_x_offset(edl, single_channel, zoom_x, conformed_w, conformed_h);
	y = (float)y / zoom_y + get_y_offset(edl, single_channel, zoom_y, conformed_w, conformed_h);
}

void Canvas::output_to_canvas(EDL *edl, int single_channel, float &x, float &y)
{
	float zoom_x, zoom_y, conformed_w, conformed_h;
	get_zooms(edl, single_channel, zoom_x, zoom_y, conformed_w, conformed_h);

//printf("Canvas::output_to_canvas x=%f zoom_x=%f x_offset=%f\n", x, zoom_x, get_x_offset(edl, single_channel, zoom_x, conformed_w));

	x = (float)zoom_x * (x - get_x_offset(edl, single_channel, zoom_x, conformed_w, conformed_h));
	y = (float)zoom_y * (y - get_y_offset(edl, single_channel, zoom_y, conformed_w, conformed_h));
}



void Canvas::get_transfers(EDL *edl, 
	int &in_x, 
	int &in_y, 
	int &in_w, 
	int &in_h,
	int &out_x, 
	int &out_y, 
	int &out_w, 
	int &out_h,
	int canvas_w,
	int canvas_h)
{
// printf("Canvas::get_transfers %d %d\n", canvas_w, 
// 		canvas_h);
	if(canvas_w < 0) canvas_w = canvas->get_w();
	if(canvas_h < 0) canvas_h = canvas->get_h();

	if(use_scrollbars)
	{
		float in_x1, in_y1, in_x2, in_y2;
		float out_x1, out_y1, out_x2, out_y2;
		float zoom_x, zoom_y, conformed_w, conformed_h;

		get_zooms(edl, 0, zoom_x, zoom_y, conformed_w, conformed_h);
		out_x1 = 0;
		out_y1 = 0;
		out_x2 = canvas_w;
		out_y2 = canvas_h;
		in_x1 = 0;
		in_y1 = 0;
		in_x2 = canvas_w;
		in_y2 = canvas_h;

		canvas_to_output(edl, 0, in_x1, in_y1);
		canvas_to_output(edl, 0, in_x2, in_y2);

// printf("Canvas::get_transfers 1 %.0f %.0f %.0f %.0f -> %.0f %.0f %.0f %.0f\n",
// 			in_x1, in_y1, in_x2, in_y2, out_x1, out_y1, out_x2, out_y2);

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
// printf("Canvas::get_transfers 2 %.0f %.0f %.0f %.0f -> %.0f %.0f %.0f %.0f\n",
// 			in_x1, in_y1, in_x2, in_y2, out_x1, out_y1, out_x2, out_y2);

		in_x = (int)in_x1;
		in_y = (int)in_y1;
		in_w = (int)(in_x2 - in_x1);
		in_h = (int)(in_y2 - in_y1);
		out_x = (int)out_x1;
		out_y = (int)out_y1;
		out_w = (int)(out_x2 - out_x1);
		out_h = (int)(out_y2 - out_y1);

// Center on canvas
//		if(!scrollbars_exist())
//		{
//			out_x = canvas_w / 2 - out_w / 2;
//			out_y = canvas_h / 2 - out_h / 2;
//		}

// printf("Canvas::get_transfers 2 %d %d %d %d -> %d %d %d %d\n",in_x, 
// 			in_y, 
// 			in_w, 
// 			in_h,
// 			out_x, 
// 			out_y, 
// 			out_w, 
// 			out_h);
	}
	else
	{
		out_x = 0;
		out_y = 0;
		out_w = canvas_w;
		out_h = canvas_h;

		if(edl)
		{
			if((float)out_w / out_h > edl->get_aspect_ratio())
			{
				out_w = (int)(out_h * edl->get_aspect_ratio() + 0.5);
				out_x = canvas_w / 2 - out_w / 2;
			}
			else
			{
				out_h = (int)(out_w / edl->get_aspect_ratio() + 0.5);
				out_y = canvas_h / 2 - out_h / 2;
			}
			in_x = 0;
			in_y = 0;
			in_w = get_output_w(edl);
			in_h = get_output_h(edl);
		}
		else
		{
			in_x = 0;
			in_y = 0;
			in_w = this->output_w;
			in_h = this->output_h;
		}
	}

	in_x = MAX(0, in_x);
	in_y = MAX(0, in_y);
	in_w = MAX(0, in_w);
	in_h = MAX(0, in_h);
	out_x = MAX(0, out_x);
	out_y = MAX(0, out_y);
	out_w = MAX(0, out_w);
	out_h = MAX(0, out_h);
}

int Canvas::scrollbars_exist()
{
	return(use_scrollbars && (xscroll || yscroll));
}

int Canvas::get_output_w(EDL *edl)
{
	if(use_scrollbars)
		return edl->calculate_output_w(0);
	else
		return edl->session->output_w;
}

int Canvas::get_output_h(EDL *edl)
{
	if(edl)
	{
		if(use_scrollbars)
			return edl->calculate_output_h(0);
		else
			return edl->session->output_h;
	}
}



void Canvas::get_scrollbars(EDL *edl, 
	int &canvas_x, 
	int &canvas_y, 
	int &canvas_w, 
	int &canvas_h)
{
	int need_xscroll = 0;
	int need_yscroll = 0;
//	int done = 0;
	float zoom_x, zoom_y, conformed_w, conformed_h;

	if(edl)
	{
		w_needed = edl->calculate_output_w(0);
		h_needed = edl->calculate_output_h(0);
		w_visible = w_needed;
		h_visible = h_needed;
	}
//printf("Canvas::get_scrollbars 1 %d %d\n", get_xscroll(), get_yscroll());

	if(use_scrollbars)
	{
		w_needed = edl->calculate_output_w(0);
		h_needed = edl->calculate_output_h(0);
		get_zooms(edl, 0, zoom_x, zoom_y, conformed_w, conformed_h);
//printf("Canvas::get_scrollbars 2 %d %d\n", get_xscroll(), get_yscroll());

//		while(!done)
//		{
			w_visible = (int)(canvas_w / zoom_x);
			h_visible = (int)(canvas_h / zoom_y);
//			done = 1;

//			if(w_needed > w_visible)
			if(1)
			{
				if(!need_xscroll)
				{
					need_xscroll = 1;
					canvas_h -= BC_ScrollBar::get_span(SCROLL_HORIZ);
//					done = 0;
				}
			}
			else
				need_xscroll = 0;

//			if(h_needed > h_visible)
			if(1)
			{
				if(!need_yscroll)
				{
					need_yscroll = 1;
					canvas_w -= BC_ScrollBar::get_span(SCROLL_VERT);
//					done = 0;
				}
			}
			else
				need_yscroll = 0;
//		}
//printf("Canvas::get_scrollbars %d %d %d %d %d %d\n", canvas_w, canvas_h, w_needed, h_needed, w_visible, h_visible);
//printf("Canvas::get_scrollbars 3 %d %d\n", get_xscroll(), get_yscroll());

		w_visible = (int)(canvas_w / zoom_x);
		h_visible = (int)(canvas_h / zoom_y);
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
//printf("Canvas::get_scrollbars 4 %d %d\n", get_xscroll(), get_yscroll());

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

		if(yscroll->get_length() != edl->calculate_output_h(0) ||
			yscroll->get_handlelength() != h_visible)
			yscroll->update_length(h_needed, get_yscroll(), h_visible);
	}
	else
	{
		if(yscroll) delete yscroll;
		yscroll = 0;
	}
//printf("Canvas::get_scrollbars 5 %d %d\n", get_xscroll(), get_yscroll());
}

void Canvas::reposition_window(EDL *edl, int x, int y, int w, int h)
{
	this->x = x;
	this->y = y;
	this->w = w;
	this->h = h;
	int view_x = x, view_y = y, view_w = w, view_h = h;
//printf("Canvas::reposition_window 1\n");
	get_scrollbars(edl, view_x, view_y, view_w, view_h);
//printf("Canvas::reposition_window %d %d %d %d\n", view_x, view_y, view_w, view_h);
	canvas->reposition_window(view_x, view_y, view_w, view_h);
	draw_refresh();
//printf("Canvas::reposition_window 2\n");
}

void Canvas::set_cursor(int cursor)
{
	canvas->set_cursor(cursor);
}

int Canvas::get_cursor_x()
{
	return canvas->get_cursor_x();
}

int Canvas::get_cursor_y()
{
	return canvas->get_cursor_y();
}

int Canvas::get_buttonpress()
{
	return canvas->get_buttonpress();
}


int Canvas::create_objects(EDL *edl)
{
	int view_x = x, view_y = y, view_w = w, view_h = h;
	get_scrollbars(edl, view_x, view_y, view_w, view_h);

	subwindow->add_subwindow(canvas = new CanvasOutput(edl, 
		this, 
		view_x, 
		view_y, 
		view_w, 
		view_h));

	subwindow->add_subwindow(canvas_menu = new CanvasPopup(this));
	canvas_menu->create_objects();

	return 0;
}

int Canvas::button_press_event()
{
	int result = 0;

	if(canvas->get_buttonpress() == 3)
	{
		canvas_menu->activate_menu();
		result = 1;
	}
	
	return result;
}




CanvasOutput::CanvasOutput(EDL *edl, 
	Canvas *canvas,
    int x,
    int y,
    int w,
    int h)
 : BC_SubWindow(x, y, w, h, BLACK)
{
	this->canvas = canvas;
	cursor_inside = 0;
}

CanvasOutput::~CanvasOutput()
{
}

int CanvasOutput::handle_event()
{
	return 1;
}

int CanvasOutput::cursor_leave_event()
{
	int result = 0;
	if(cursor_inside) result = canvas->cursor_leave_event();
	cursor_inside = 0;
	return result;
}

int CanvasOutput::cursor_enter_event()
{
	int result = 0;
	if(is_event_win() && BC_WindowBase::cursor_inside())
	{
		cursor_inside = 1;
		result = canvas->cursor_enter_event();
	}
	return result;
}

int CanvasOutput::button_press_event()
{
	if(is_event_win() && BC_WindowBase::cursor_inside())
	{
		return canvas->button_press_event();
	}
	return 0;
}

int CanvasOutput::button_release_event()
{
	return canvas->button_release_event();
}

int CanvasOutput::cursor_motion_event()
{
	return canvas->cursor_motion_event();
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

CanvasXScroll::~CanvasXScroll()
{
}

int CanvasXScroll::handle_event()
{
//printf("CanvasXScroll::handle_event %d %d %d\n", get_length(), get_value(), get_handlelength());
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

CanvasYScroll::~CanvasYScroll()
{
}

int CanvasYScroll::handle_event()
{
//printf("CanvasYScroll::handle_event %d %d\n", get_value(), get_length());
	canvas->update_zoom(canvas->get_xscroll(), get_value(), canvas->get_zoom());
	canvas->draw_refresh();
	return 1;
}




CanvasPopup::CanvasPopup(Canvas *canvas)
 : BC_PopupMenu(0, 
		0, 
		0, 
		"", 
		0)
{
	this->canvas = canvas;
}

CanvasPopup::~CanvasPopup()
{
}

void CanvasPopup::create_objects()
{
	add_item(new CanvasPopupSize(canvas, "Zoom 25%", 0.25));
	add_item(new CanvasPopupSize(canvas, "Zoom 50%", 0.5));
	add_item(new CanvasPopupSize(canvas, "Zoom 100%", 1.0));
	add_item(new CanvasPopupSize(canvas, "Zoom 200%", 2.0));
	if(canvas->use_cwindow)
	{
		add_item(new CanvasPopupResetCamera(canvas));
		add_item(new CanvasPopupResetProjector(canvas));
	}
	if(canvas->use_rwindow)
	{
		add_item(new CanvasPopupResetTranslation(canvas));
	}
	if(canvas->use_vwindow)
	{
		add_item(new CanvasPopupRemoveSource(canvas));
	}
}



CanvasPopupSize::CanvasPopupSize(Canvas *canvas, char *text, float percentage)
 : BC_MenuItem(text)
{
	this->canvas = canvas;
	this->percentage = percentage;
}
CanvasPopupSize::~CanvasPopupSize()
{
}
int CanvasPopupSize::handle_event()
{
	canvas->zoom_resize_window(percentage);
	return 1;
}



CanvasPopupResetCamera::CanvasPopupResetCamera(Canvas *canvas)
 : BC_MenuItem("Reset camera")
{
	this->canvas = canvas;
}
int CanvasPopupResetCamera::handle_event()
{
	canvas->reset_camera();
	return 1;
}



CanvasPopupResetProjector::CanvasPopupResetProjector(Canvas *canvas)
 : BC_MenuItem("Reset projector")
{
	this->canvas = canvas;
}
int CanvasPopupResetProjector::handle_event()
{
	canvas->reset_projector();
	return 1;
}



CanvasPopupResetTranslation::CanvasPopupResetTranslation(Canvas *canvas)
 : BC_MenuItem("Reset translation")
{
	this->canvas = canvas;
}
int CanvasPopupResetTranslation::handle_event()
{
	canvas->reset_translation();
	return 1;
}




CanvasPopupRemoveSource::CanvasPopupRemoveSource(Canvas *canvas)
 : BC_MenuItem("Close source")
{
	this->canvas = canvas;
}
int CanvasPopupRemoveSource::handle_event()
{
	canvas->close_source();
	return 1;
}


