#include "bcbutton.h"
#include "bcresources.h"
#include "bcpixmap.h"
#include "colors.h"
#include "fonts.h"
#include "keys.h"
#include "vframe.h"


#include <string.h>
#include <unistd.h>

#define BUTTON_UP 0
#define BUTTON_UPHI 1
#define BUTTON_DOWNHI 2

BC_Button::BC_Button(int x, 
	int y, 
	VFrame **data)
 : BC_SubWindow(x, y, 0, 0, -1)
{
	this->data = data;
	for(int i = 0; i < 3; i++) images[i] = 0;
	if(!data) printf("BC_Button::BC_Button data == 0\n");
	status = BUTTON_UP;
	this->w_argument = 0;
	underline_number = -1;
}

BC_Button::BC_Button(int x, 
	int y, 
	int w, 
	VFrame **data)
 : BC_SubWindow(x, y, 0, 0, -1)
{
	this->data = data;
	this->w_argument = w;
	for(int i = 0; i < 3; i++) images[i] = 0;
	if(!data) printf("BC_Button::BC_Button data == 0\n");
	status = BUTTON_UP;
	underline_number = -1;
}


BC_Button::~BC_Button()
{
	for(int i = 0; i < 3; i++) if(images[i]) delete images[i];
}



int BC_Button::initialize()
{
// Get the image
	set_images(data);

// Create the subwindow
	BC_SubWindow::initialize();

// Display the bitmap
	draw_face();
	return 0;
}

int BC_Button::reposition_window(int x, int y)
{
	BC_WindowBase::reposition_window(x, y);
	draw_face();
	return 0;
}


int BC_Button::update_bitmaps(VFrame **data)
{
	this->data = data;
	set_images(data);
	draw_top_background(parent_window, 0, 0, w, h);
	draw_face();
	return 0;
}

void BC_Button::set_underline(int number)
{
	this->underline_number = number;
}

int BC_Button::set_images(VFrame **data)
{
	for(int i = 0; i < 3; i++)
	{
		if(images[i]) delete images[i];
		images[i] = new BC_Pixmap(parent_window, data[i], PIXMAP_ALPHA);
	}

	if(w_argument > 0)
		w = w_argument;
	else
		w = images[BUTTON_UP]->get_w();

	h = images[BUTTON_UP]->get_h();
	return 0;
}

int BC_Button::draw_face()
{
	draw_top_background(parent_window, 0, 0, w, h);
	images[status]->write_drawable(pixmap, 
			0, 
			0,
			w,
			h,
			0,
			0);
	flash();
	return 0;
}

int BC_Button::repeat_event(int64_t duration)
{
	if(duration == top_level->get_resources()->tooltip_delay &&
		tooltip_text[0] != 0 &&
		status == BUTTON_UPHI &&
		!tooltip_done)
	{
		show_tooltip();
		tooltip_done = 1;
		return 1;
	}
	return 0;
}

int BC_Button::cursor_enter_event()
{
	if(top_level->event_win == win)
	{
		tooltip_done = 0;
		if(top_level->button_down)
		{
			status = BUTTON_DOWNHI;
		}
		else
		if(status == BUTTON_UP) status = BUTTON_UPHI;
		draw_face();
	}
	return 0;
}

int BC_Button::cursor_leave_event()
{
	if(status == BUTTON_UPHI)
	{
		status = BUTTON_UP;
		draw_face();
		hide_tooltip();
	}
	return 0;
}

int BC_Button::button_press_event()
{
	if(top_level->event_win == win)
	{
		top_level->hide_tooltip();
		if(status == BUTTON_UPHI || status == BUTTON_UP) status = BUTTON_DOWNHI;
		draw_face();
		return 1;
	}
	return 0;
}

int BC_Button::button_release_event()
{
//printf("BC_Button::button_release_event 1\n");
	if(top_level->event_win == win)
	{
		hide_tooltip();
//printf("BC_Button::button_release_event 2\n");
		if(status == BUTTON_DOWNHI) 
		{
//printf("BC_Button::button_release_event 3\n");
			status = BUTTON_UPHI;
			draw_face();

			if(cursor_inside())
			{
//printf("BC_Button::button_release_event 4\n");
				handle_event();
				return 1;
			}
		}
	}
	return 0;
}

int BC_Button::cursor_motion_event()
{
	if(top_level->button_down && top_level->event_win == win && 
		status == BUTTON_DOWNHI && !cursor_inside())
	{
		status = BUTTON_UP;
		draw_face();
	}
	return 0;
}


BC_OKButton::BC_OKButton(int x, int y)
 : BC_Button(x, y, 
 	BC_WindowBase::get_resources()->ok_images)
{
}

BC_OKButton::BC_OKButton(BC_WindowBase *parent_window)
 : BC_Button(10, 
 	parent_window->get_h() - BC_WindowBase::get_resources()->cancel_images[0]->get_h() - 10, 
 	BC_WindowBase::get_resources()->ok_images)
{
}

int BC_OKButton::handle_event()
{
	get_top_level()->set_done(0);
	return 0;
}

int BC_OKButton::resize_event(int w, int h)
{
	reposition_window(10,
		h - BC_WindowBase::get_resources()->cancel_images[0]->get_h() - 10);
	return 1;
}

int BC_OKButton::keypress_event()
{
	if(get_keypress() == RETURN) return handle_event();
	return 0;
}

BC_CancelButton::BC_CancelButton(int x, int y)
 : BC_Button(x, y, 
 	BC_WindowBase::get_resources()->cancel_images)
{
}

BC_CancelButton::BC_CancelButton(BC_WindowBase *parent_window)
 : BC_Button(parent_window->get_w() - BC_WindowBase::get_resources()->cancel_images[0]->get_w() - 10, 
 	parent_window->get_h() - BC_WindowBase::get_resources()->cancel_images[0]->get_h() - 10, 
 	BC_WindowBase::get_resources()->cancel_images)
{
}

int BC_CancelButton::handle_event()
{
	get_top_level()->set_done(1);
	return 1;
}

int BC_CancelButton::resize_event(int w,int h)
{
	reposition_window(w - BC_WindowBase::get_resources()->cancel_images[0]->get_w() - 10, 
	 	h - BC_WindowBase::get_resources()->cancel_images[0]->get_h() - 10);
	return 1;
}

int BC_CancelButton::keypress_event()
{
	if(get_keypress() == ESC) return handle_event();
	return 0;
}




#define LEFT_DN  0
#define LEFT_HI  1
#define LEFT_UP  2
#define MID_DN   3
#define MID_HI   4
#define MID_UP   5
#define RIGHT_DN 6
#define RIGHT_HI 7
#define RIGHT_UP 8

BC_GenericButton::BC_GenericButton(int x, int y, char *text, VFrame **data)
 : BC_Button(x, 
 	y, 
	data ? data : BC_WindowBase::get_resources()->generic_button_images)
{
	strcpy(this->text, text);
}

BC_GenericButton::BC_GenericButton(int x, int y, int w, char *text, VFrame **data)
 : BC_Button(x, 
 	y, 
	w,
	data ? data : BC_WindowBase::get_resources()->generic_button_images)
{
	strcpy(this->text, text);
}

int BC_GenericButton::set_images(VFrame **data)
{
	for(int i = 0; i < 3; i++)
	{
		if(images[i]) delete images[i];
		images[i] = new BC_Pixmap(parent_window, data[i], PIXMAP_ALPHA);
	}

	if(w_argument)
		w = w_argument;
	else
		w = get_text_width(MEDIUMFONT, text) + images[BUTTON_UP]->get_w() / 3;


	h = images[BUTTON_UP]->get_h();
	return 0;
}

int BC_GenericButton::draw_face()
{
	int x, y, w;
	draw_top_background(parent_window, 0, 0, get_w(), get_h());
	draw_3segmenth(0, 0, get_w(), images[status]);

	set_color(BLACK);
	set_font(MEDIUMFONT);
	y = (int)((float)get_h() / 2 + get_text_ascent(MEDIUMFONT) / 2 - 2);
	w = get_text_width(current_font, text, strlen(text));
	x = get_w() / 2 - w / 2;
	draw_text(x, 
		y, 
		text);

	if(underline_number >= 0)
	{
		y++;
		int x1 = get_text_width(current_font, text, underline_number) + x;
		int x2 = get_text_width(current_font, text, underline_number + 1) + x;
		draw_line(x1, y, x2, y);
		draw_line(x1, y + 1, (x2 + x1) / 2, y + 1);
	}

	flash();
	return 0;
}









