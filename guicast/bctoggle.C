#include "bcpixmap.h"
#include "bcresources.h"
#include "bctoggle.h"
#include "colors.h"
#include "fonts.h"

#include <string.h>

BC_Toggle::BC_Toggle(int x, int y, 
		VFrame **data, 
		int value, 
		char *caption,
		int bottom_justify, 
		int font,
		int color)
 : BC_SubWindow(x, y, 0, 0, -1)
{
	this->data = data;
	for(int i = 0; i < 5; i++)
		images[i] = 0;
	status = value ? BC_Toggle::TOGGLE_CHECKED : BC_Toggle::TOGGLE_UP;
	this->value = value;
	strcpy(this->caption, caption);
	this->bottom_justify = bottom_justify;
	this->font = font;
	this->color = color;
	select_drag = 0;
	enabled = 1;
}


BC_Toggle::~BC_Toggle()
{
	for(int i = 0; i < 5; i++) if(images[i]) delete images[i];
}


int BC_Toggle::initialize()
{
// Get the image
	set_images(data);
	w = images[0]->get_w();
	h = images[0]->get_h();
	toggle_x = 0;
	toggle_y = 0;
	text_y = 0;
	text_x = w + 5;

// Expand subwindow for text
	if(has_caption())
	{
		text_w = get_text_width(MEDIUMFONT, caption);
		text_h = get_text_height(MEDIUMFONT);
		if(text_h > h)
		{
			toggle_y = (text_h - h) >> 1;
			h = text_h;
		}
		else
			text_y = (h - text_h) >> 1;

		if(bottom_justify)
		{
			text_y = h - text_h;
			text_line = h - get_text_descent(MEDIUMFONT);
		}
		else
			text_line = text_y + get_text_ascent(MEDIUMFONT);
		
		w = text_x + text_w;
	}

// Create the subwindow
	BC_SubWindow::initialize();

// Display the bitmap
	draw_face();
	return 0;
}

int BC_Toggle::set_images(VFrame **data)
{
	for(int i = 0; i < 5; i++)
	{
		if(images[i]) delete images[i];
		images[i] = new BC_Pixmap(top_level, data[i], PIXMAP_ALPHA);
	}
	return 0;
}

void BC_Toggle::set_select_drag(int value)
{
	this->select_drag = value;
}

int BC_Toggle::draw_face()
{
	draw_top_background(parent_window, 0, 0, get_w(), get_h());
	if(has_caption())
	{
		if(enabled &&
			(status == BC_Toggle::TOGGLE_UPHI || status == BC_Toggle::TOGGLE_DOWN || status == BC_Toggle::TOGGLE_CHECKEDHI))
		{
			set_color(LTGREY);
			draw_box(text_x, text_line - get_text_ascent(MEDIUMFONT), get_w() - text_x, get_text_height(MEDIUMFONT));
		}

		set_opaque();
		if(enabled)
			set_color(color);
		else
			set_color(MEGREY);
		set_font(font);
		draw_text(text_x, text_line, caption);
	}

	draw_pixmap(images[status]);
	flash();
	return 0;
}

void BC_Toggle::enable()
{
	enabled = 1;
	draw_face();
}

void BC_Toggle::disable()
{
	enabled = 0;
	draw_face();
}

void BC_Toggle::set_status(int value)
{
	this->status = value;
}


int BC_Toggle::repeat_event(int64_t duration)
{
	if(duration == top_level->get_resources()->tooltip_delay &&
		tooltip_text[0] != 0 &&
		(status == BC_Toggle::TOGGLE_UPHI || status == BC_Toggle::TOGGLE_CHECKEDHI) &&
		!tooltip_done)
	{
		show_tooltip();
		tooltip_done = 1;
		return 1;
	}
	return 0;
}

int BC_Toggle::cursor_enter_event()
{
	if(top_level->event_win == win && enabled)
	{
		tooltip_done = 0;
		if(top_level->button_down)
			status = BC_Toggle::TOGGLE_DOWN;
		else
			status = value ? BC_Toggle::TOGGLE_CHECKEDHI : BC_Toggle::TOGGLE_UPHI;
		draw_face();
	}
	return 0;
}

int BC_Toggle::cursor_leave_event()
{
	hide_tooltip();
	if(!value)
	{
		status = BC_Toggle::TOGGLE_UP;
		draw_face();
	}
	else
	{
		status = BC_Toggle::TOGGLE_CHECKED;
		draw_face();
	}
	return 0;
}

int BC_Toggle::button_press_event()
{
	hide_tooltip();
	if(top_level->event_win == win && get_buttonpress() == 1 && enabled)
	{
		status = BC_Toggle::TOGGLE_DOWN;
		top_level->toggle_value = !this->value;
		draw_face();
		return 1;
	}
	return 0;
}

int BC_Toggle::button_release_event()
{
	hide_tooltip();
	if(top_level->event_win == win && status == BC_Toggle::TOGGLE_DOWN)
	{
		if((!value && !select_drag) || (value && select_drag))
		{
			status = BC_Toggle::TOGGLE_CHECKEDHI;
			value = 1;
		}
		else
		{
			status = BC_Toggle::TOGGLE_UPHI;
			value = 0;
		}
		draw_face();
		return handle_event();
	}
	return 0;
}

int BC_Toggle::cursor_motion_event()
{
	if(select_drag) return 0;
	if(top_level->button_down && 
		top_level->event_win == win && 
		!cursor_inside())
	{
		if(status == BC_Toggle::TOGGLE_DOWN)
		{
			status = BC_Toggle::TOGGLE_UP;
			draw_face();
		}
		else
		if(status == BC_Toggle::TOGGLE_UPHI)
		{
			status = BC_Toggle::TOGGLE_CHECKEDHI;
			draw_face();
		}
	}
	return 0;
}

int BC_Toggle::get_value()
{
	return value;
}

int BC_Toggle::set_value(int value, int draw)
{
	if(value != this->value)
	{
		this->value = value;
		if(value) 
		{
			switch(status)
			{
				case BC_Toggle::TOGGLE_UP:
					status = BC_Toggle::TOGGLE_CHECKED;
					break;
				case BC_Toggle::TOGGLE_UPHI:
					status = BC_Toggle::TOGGLE_CHECKEDHI;
					break;
			}
		}
		else
		switch(status)
		{
			case BC_Toggle::TOGGLE_CHECKED:
				status = BC_Toggle::TOGGLE_UP;
				break;
			case BC_Toggle::TOGGLE_CHECKEDHI:
				status = BC_Toggle::TOGGLE_UPHI;
				break;
		}
		if(draw) draw_face();
	}
	return 0;
}

int BC_Toggle::update(int value, int draw)
{
	return set_value(value, draw);
}

void BC_Toggle::reposition_window(int x, int y)
{
	BC_WindowBase::reposition_window(x, y);
	draw_face();
}


int BC_Toggle::has_caption()
{
	return (caption != 0 && caption[0] != 0);
}

BC_Radial::BC_Radial(int x, 
	int y, 
	int value, 
	char *caption, 
	int font,
	int color)
 : BC_Toggle(x, 
 	y, 
	BC_WindowBase::get_resources()->radial_images, 
	value, 
	caption, 
	0, 
	font,
	color)
{
}

BC_CheckBox::BC_CheckBox(int x, 
	int y, 
	int value, 
	char *caption, 
	int font,
	int color)
 : BC_Toggle(x, 
 	y, 
	BC_WindowBase::get_resources()->checkbox_images, 
	value, 
	caption, 
	1, 
	font,
	color)
{
	this->value = 0;
}

BC_CheckBox::BC_CheckBox(int x, 
	int y, 
	int *value, 
	char *caption, 
	int font,
	int color)
 : BC_Toggle(x, 
 	y, 
	BC_WindowBase::get_resources()->checkbox_images, 
	*value, 
	caption, 
	1, 
	font,
	color)
{
	this->value = value;
}

int BC_CheckBox::handle_event()
{
	*value = get_value();
	return 1;
}




BC_Label::BC_Label(int x, 
	int y, 
	int value, 
	int font,
	int color)
 : BC_Toggle(x, 
 	y, 
	BC_WindowBase::get_resources()->label_images, 
	value, 
	"", 
	0, 
	font,
	color)
{
}
