// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bcdisplayinfo.h"
#include "bcresources.h"
#include "bcsignals.h"
#include "bctitle.h"
#include "colorpicker.h"
#include "colorspaces.h"
#include "condition.h"
#include "language.h"
#include "mutex.h"
#include "mwindow.inc"
#include "vframe.h"

#include <string.h>
#include <unistd.h>


ColorThread::ColorThread(int do_alpha, const char *title, VFrame* icon)
 : Thread()
{
	window = 0;
	this->title = title;
	this->do_alpha = do_alpha;
	this->icon = icon;
	mutex = new Mutex("ColorThread::mutex");
	completion = new Condition(1, "ColorThread::completion");
}

ColorThread::~ColorThread()
{
	if(running())
	{
		window->set_done(0);
		completion->lock("ColorThread::~ColorThread");
		completion->unlock();
	}
	if(icon)
		delete icon;
	delete mutex;
	delete completion;
}

void ColorThread::start_window(int r, int g, int b, int a)
{
	mutex->lock("ColorThread::start_window");
	red = CLIP(r, 0, 0xffff);
	green = CLIP(g, 0, 0xffff);
	blue = CLIP(b, 0, 0xffff);
	alpha = CLIP(a, 0, 0xffff);
	mutex->unlock();

	if(!running())
	{
		completion->lock("ColorThread::start_window");
		Thread::start();
	}
	else
	{
		window->raise_window();
		window->flush();
	}
}

void ColorThread::start_window(int output, int alpha)
{
	start_window((output >> 8) & 0xff00, output & 0xff00, (output << 8) & 0xff00,
		alpha << 8);
}

void ColorThread::run()
{
	char window_title[BCTEXTLEN];
	int x, y;

	if(title)
		strcpy(window_title, title);
	else
		strcpy(window_title, _("Color Picker"));
	strcat(window_title, " - " PROGRAM_NAME);

	BC_Resources::get_abs_cursor(&x, &y);
	mutex->lock("ColorThread::run 1");
	window = new ColorWindow(this, x - 200, y - 200, window_title);
	mutex->unlock();
	update_gui(red, green, blue, alpha);
	window->run_window();
	mutex->lock("ColorThread::run 2");
	delete window;
	window = 0;
	mutex->unlock();
	completion->unlock();
}

void ColorThread::update_gui(int r, int g, int b, int a)
{
	mutex->lock("ColorThread::update_gui");
	if(window)
	{
		red = CLIP(r, 0, 0xffff);
		green = CLIP(g, 0, 0xffff);
		blue = CLIP(b, 0, 0xffff);
		alpha = CLIP(a, 0, 0xffff);
		window->change_values();
		window->update_display();
	}
	mutex->unlock();
}

void ColorThread::update_gui(int output, int alpha)
{
	update_gui((output >> 8) & 0xff00, output & 0xff00, (output << 8) & 0xff00,
		alpha << 8);
}


ColorWindow::ColorWindow(ColorThread *thread, int x, int y, const char *title)
 : BC_Window(title, x, y, 410, 320, 410, 320,
	0, 0, 1, -1, 0, 1, WINDOW_UTF8)
{
	int init_x = 10, init_y = 10;
	x = y = 10;
	this->thread = thread;

	change_values();

	if(thread->icon)
		set_icon(thread->icon);

	add_tool(wheel = new PaletteWheel(this, x, y));
	wheel->show();

	x += 180;
	add_tool(wheel_value = new PaletteWheelValue(this, x, y));

	y += 180;
	x = init_x; 
	add_tool(output = new PaletteOutput(this, x, y));

	x += 240; y = init_y;
	add_tool(new BC_Title(x, y, _("Hue"), SMALLFONT));
	y += 15;

	add_tool(hue = new PaletteHue(this, x, y));
	y += 30;

	add_tool(new BC_Title(x, y, _("Saturation"), SMALLFONT));
	y += 15;

	add_tool(saturation = new PaletteFSlider(this, x, y, &sat));
	y += 30;

	add_tool(new BC_Title(x, y, _("Value"), SMALLFONT));
	y += 15;

	add_tool(value = new PaletteFSlider(this, x, y, &val));
	y += 30;

	add_tool(new BC_Title(x, y, _("Red"), SMALLFONT));
	y += 15;

	add_tool(red = new PaletteISlider(this, x, y, &thread->red));
	y += 30;

	add_tool(new BC_Title(x, y, _("Green"), SMALLFONT));
	y += 15;

	add_tool(green = new PaletteISlider(this, x, y, &thread->green));
	y += 30;

	add_tool(new BC_Title(x, y, _("Blue"), SMALLFONT));
	y += 15;

	add_tool(blue = new PaletteISlider(this, x, y, &thread->blue));

	if(thread->do_alpha)
	{
		y += 30;
		add_tool(new BC_Title(x, y, _("Alpha"), SMALLFONT));
		y += 15;
		add_tool(alpha = new PaletteISlider(this, x, y, &thread->alpha));
	}
	else
		alpha = 0;

	show_window();
	flush();
}

void ColorWindow::change_values()
{
	ColorSpaces::rgb_to_hsv(thread->red, thread->green, thread->blue,
		&hueval, &sat, &val);
}

void ColorWindow::close_event()
{
	set_done(0);
}

void ColorWindow::update_rgb()
{
	ColorSpaces::rgb_to_hsv(red->get_value(),
		green->get_value(),
		blue->get_value(),
		&hueval, &sat, &val);
	if(alpha)
		thread->alpha = alpha->get_value();
	update_display();
}

void ColorWindow::update_display()
{
	if(hueval < 0) hueval = 0;
	if(hueval > 360) hueval = 360;
	if(sat < 0) sat = 0;
	if(sat > 1) sat = 1;
	if(val < 0) val = 0;
	if(val > 1) val = 1;

	wheel->draw(wheel->oldhue, wheel->oldsaturation);
	wheel->oldhue = hueval;
	wheel->oldsaturation = sat;
	wheel->draw(hueval, sat);
	wheel->flash();
	wheel_value->draw(hueval, sat, val);
	wheel_value->flash();
	output->draw();
	output->flash();
	hue->update(hueval);
	saturation->update(sat);
	value->update(val);

	ColorSpaces::hsv_to_rgb(&thread->red, &thread->green, &thread->blue,
		hueval, sat, val);
	red->update(thread->red);
	green->update(thread->green);
	blue->update(thread->blue);
	if(thread->do_alpha)
		alpha->update(thread->alpha);
}

int ColorWindow::handle_event()
{
	if(!thread->handle_new_color(thread->red, thread->green, thread->blue, thread->alpha))
	{
		int result = (((thread->red << 8) & 0xff0000) |
			(thread->green & 0xff00) | (thread->blue >> 8));
		thread->handle_new_color(result, thread->alpha >> 8);
	}
	return 1;
}


PaletteWheel::PaletteWheel(ColorWindow *window, int x, int y)
 : BC_SubWindow(x, y, 170, 170)
{
	this->window = window;
	oldhue = 0;
	oldsaturation = 0;
	button_down = 0;
}

void PaletteWheel::show()
{
// Upper right
	int h;
	double s;
	double v = 1;
	int r, g, b;
	int x1, y1, x2, y2;
	int distance;
	int default_r, default_g, default_b;
	VFrame frame(0, get_w(), get_h(), BC_RGBA8888);

	x1 = get_w() / 2;
	y1 = get_h() / 2;
	default_r = (get_resources()->get_bg_color() & 0xff0000) >> 16;
	default_g = (get_resources()->get_bg_color() & 0xff00) >> 8;
	default_b = (get_resources()->get_bg_color() & 0xff);

	for(y2 = 0; y2 < get_h(); y2++)
	{
		unsigned char *row = frame.get_row_ptr(y2);

		for(x2 = 0; x2 < get_w(); x2++)
		{
			distance = round(sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1)));
			if(distance > x1)
			{
				row[x2 * 4] = default_r;
				row[x2 * 4 + 1] = default_g;
				row[x2 * 4 + 2] = default_b;
				row[x2 * 4 + 3] = 0;
			}
			else
			{
				h = get_angle(x1, y1, x2, y2);
				s = (double)distance / x1;
				ColorSpaces::hsv_to_rgb(&r, &g, &b, h, s, v);
				row[x2 * 4] = r >> 8;
				row[x2 * 4 + 1] = g >> 8;
				row[x2 * 4 + 2] = b >> 8;
				row[x2 * 4 + 3] = 255;
			}
		}
	}

	draw_vframe(&frame,
		0, 0, get_w(), get_h(),
		0, 0, get_w(), get_h(), 0);

	oldhue = window->hueval;
	oldsaturation = window->sat;
	draw(oldhue, oldsaturation);
	flash();
}

int PaletteWheel::button_press_event()
{
	int cursor_x = get_cursor_x();
	int cursor_y = get_cursor_y();

	if(cursor_x >= 0 && cursor_x < get_w() &&
		cursor_y >= 0 && cursor_y < get_h() &&
		is_event_win())
	{
		button_down = 1;
		cursor_motion_event();
		return 1;
	}
	return 0;
}

int PaletteWheel::cursor_motion_event()
{
	int x1, y1, distance;

	if(button_down && is_event_win())
	{
		int cursor_x = get_cursor_x();
		int cursor_y = get_cursor_y();

		window->hueval = get_angle(get_w() / 2,
			get_h() / 2,
			cursor_x,
			cursor_y);
		x1 = get_w() / 2 - cursor_x;
		y1 = get_h() / 2 - cursor_y;
		distance = round(sqrt(x1 * x1 + y1 * y1));
		if(distance > get_w() / 2)
			distance = get_w() / 2;
		window->sat = (double)distance / (get_w() / 2);
		window->update_display();
		window->handle_event();
		return 1;
	}
	return 0;
}

int PaletteWheel::button_release_event()
{
	if(button_down)
	{
		button_down = 0;
		return 1;
	}
	return 0;
}

double PaletteWheel::torads(double angle)
{
	return angle / 360 * 2 * M_PI;
}

void PaletteWheel::draw(int hue, double saturation)
{
	int x, y, w, h;

	w = get_w() / 2;
	h = get_h() / 2;

	if(hue > 0 && hue < 90)
	{
		x = round(w + w * cos(torads(90 - hue)) * saturation);
		y = round(h - h * sin(torads(90 - hue)) * saturation);
	}
	else
	if(hue > 90 && hue < 180)
	{
		x = round(w + w * cos(torads(hue - 90)) * saturation);
		y = round(h + h * sin(torads(hue - 90)) * saturation);
	}
	else
	if(hue > 180 && hue < 270)
	{
		x = round(w - w * cos(torads(270 - hue)) * saturation);
		y = round(h + h * sin(torads(270 - hue)) * saturation);
	}
	else
	if(hue > 270 && hue < 360)
	{
		x = round(w - w * cos(torads(hue - 270)) * saturation);
		y = round(h - w * sin(torads(hue - 270)) * saturation);
	}
	else
	if(hue == 0) 
	{
		x = w;
		y = round(h - h * saturation);
	}
	else
	if(hue == 90)
	{
		x = round(w + w * saturation);
		y = h;
	}
	else
	if(hue == 180)
	{
		x = w;
		y = round(h + h * saturation);
	}
	else
	if(hue == 270)
	{
		x = round(w - w * saturation);
		y = h;
	}

	set_inverse();
	set_color(WHITE);
	draw_circle(x - 5, y - 5, 10, 10);
	set_opaque();
}

int PaletteWheel::get_angle(int x1, int y1, int x2, int y2)
{
	double result = atan2(x2 - x1, y1 - y2) * (360 / M_PI / 2);

	if(result < 0)
		result += 360;
	return round(result);
}

PaletteWheelValue::PaletteWheelValue(ColorWindow *window, int x, int y)
 : BC_SubWindow(x, y, 40, 170, BLACK)
{
	this->window = window;
	button_down = 0;

	frame = new VFrame(0, get_w(), get_h(), BC_RGB888);
}

PaletteWheelValue::~PaletteWheelValue()
{
	delete frame;
}

int PaletteWheelValue::button_press_event()
{
	if(get_cursor_x() >= 0 && get_cursor_x() < get_w() &&
		get_cursor_y() >= 0 && get_cursor_y() < get_h() && 
		is_event_win())
	{
		button_down = 1;
		cursor_motion_event();
		return 1;
	}
	return 0;
}

int PaletteWheelValue::cursor_motion_event()
{
	int x1, y1, distance;

	if(button_down && is_event_win())
	{
		window->val = (double)(get_h() - get_cursor_y()) / get_h();
		window->update_display();
		window->handle_event();
		return 1;
	}
	return 0;
}

int PaletteWheelValue::button_release_event()
{
	if(button_down)
	{
		button_down = 0;
		return 1;
	}
	return 0;
}

void PaletteWheelValue::draw(int hue, double saturation, double value)
{
	int i, j, r, g, b;

	for(i = get_h() - 1; i >= 0; i--)
	{
		unsigned char *row = frame->get_row_ptr(i);

		ColorSpaces::hsv_to_rgb(&r, &g, &b, hue,
			saturation, (double)(get_h() - 1 - i) / get_h());
		r >>= 8;
		g >>= 8;
		b >>= 8;
		for(j = 0; j < get_w(); j++)
		{
			row[j * 3] = r;
			row[j * 3 + 1] = g;
			row[j * 3 + 2] = b;
		}
	}
	draw_vframe(frame,
		0, 0, get_w(), get_h(),
		0, 0, get_w(), get_h(), 0);
	set_color(BLACK);
	draw_line(0, get_h() - (int)(value * get_h()),
		get_w(), get_h() - (int)(value * get_h()));
}


PaletteOutput::PaletteOutput(ColorWindow *window, int x, int y)
 : BC_SubWindow(x, y, 180, 30, BLACK)
{
	this->window = window;
}

int PaletteOutput::handle_event()
{
	return 1;
}

void PaletteOutput::draw()
{
	int r, g, b;

	ColorSpaces::hsv_to_rgb(&r, &g, &b, window->hueval, window->sat, window->val);
	set_color((r << 8) & 0xff0000 | (g & 0xff00) | ((b >> 8)));
	draw_box(0, 0, get_w(), get_h());
}


PaletteHue::PaletteHue(ColorWindow *window, int x, int y)
 : BC_ISlider(x, y, 0, 150, 200, 0, 359, window->hueval, 0)
{
	this->window = window;
}

int PaletteHue::handle_event()
{
	window->hueval = get_value();
	window->update_display();
	window->handle_event();
	return 1;
}

PaletteFSlider::PaletteFSlider(ColorWindow *window, int x, int y, double *value)
 : BC_FSlider(x, y, 0, 150, 200, 0, 1.0, *value, 0)
{
	this->window = window;
	this->value = value;
	set_precision(0.01);
}

int PaletteFSlider::handle_event()
{
	*value = get_value();
	window->update_display();
	window->handle_event();
	return 1;
}


PaletteISlider::PaletteISlider(ColorWindow *window, int x, int y, int *value)
 : BC_ISlider(x, y, 0, 150, 200, 0, 0xffff, *value, 0)
{
	this->window = window;
}

int PaletteISlider::handle_event()
{
	window->update_rgb();
	window->handle_event();
	return 1;
}
