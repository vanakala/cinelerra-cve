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

void ColorThread::start_window(int r, int g, int b, int alpha)
{
	CLAMP(r, 0, 0xffff);
	CLAMP(g, 0, 0xffff);
	CLAMP(b, 0, 0xffff);
	CLAMP(alpha, 0, 0xffff);
	start_window(((r << 8) & 0xff0000) | (g & 0xff00) | (b >> 8),
		alpha >> 8);
}

void ColorThread::start_window(int output, int alpha)
{
	mutex->lock("ColorThread::start_window 1");
	this->output = output;
	this->alpha = alpha;
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
	window = new ColorWindow(this, 
		x - 200, y - 200,
		window_title);
	mutex->unlock();
	update_gui(output, alpha);
	window->run_window();
	mutex->lock("lorThread::run 2");
	delete window;
	window = 0;
	mutex->unlock();
	completion->unlock();
}

void ColorThread::update_gui(int r, int g, int b, int alpha)
{
	CLAMP(r, 0, 0xffff);
	CLAMP(g, 0, 0xffff);
	CLAMP(b, 0, 0xffff);
	CLAMP(alpha, 0, 0xffff);
	update_gui(((r << 8) & 0xff0000) | (g & 0xff00) | (b >> 8),
		alpha >> 8);
}

void ColorThread::update_gui(int output, int alpha)
{
	mutex->lock("ColorThread::update_gui");
	if(window)
	{
		this->output = output;
		this->alpha = alpha;
		window->change_values();
		window->lock_window();
		window->update_display();
		window->unlock_window();
	}
	mutex->unlock();
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

	add_tool(saturation = new PaletteSaturation(this, x, y));
	y += 30;

	add_tool(new BC_Title(x, y, _("Value"), SMALLFONT));
	y += 15;

	add_tool(value = new PaletteValue(this, x, y));
	y += 30;

	add_tool(new BC_Title(x, y, _("Red"), SMALLFONT));
	y += 15;

	add_tool(red = new PaletteRed(this, x, y));
	y += 30;

	add_tool(new BC_Title(x, y, _("Green"), SMALLFONT));
	y += 15;

	add_tool(green = new PaletteGreen(this, x, y));
	y += 30;

	add_tool(new BC_Title(x, y, _("Blue"), SMALLFONT));
	y += 15;

	add_tool(blue = new PaletteBlue(this, x, y));

	if(thread->do_alpha)
	{
		y += 30;
		add_tool(new BC_Title(x, y, _("Alpha"), SMALLFONT));
		y += 15;
		add_tool(alpha = new PaletteAlpha(this, x, y));
	}

	show_window();
	flush();
}

void ColorWindow::change_values()
{
	r = (float)((thread->output & 0xff0000) >> 16) / 255;
	g = (float)((thread->output & 0xff00) >> 8) / 255;
	b = (float)((thread->output & 0xff)) / 255;
	ColorSpaces::rgb_to_hsv(r, g, b, h, s, v);
	a = (float)thread->alpha / 255;
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
		h, s, v);
	update_display();
}

void ColorWindow::update_display()
{
	float r, g, b;
	if(h < 0) h = 0;
	if(h > 360) h = 360;
	if(s < 0) s = 0;
	if(s > 1) s = 1;
	if(v < 0) v = 0;
	if(v > 1) v = 1;
	if(a < 0) a = 0;
	if(a > 1) a = 1;

	wheel->draw(wheel->oldhue, wheel->oldsaturation);
	wheel->oldhue = h;
	wheel->oldsaturation = s;
	wheel->draw(h, s);
	wheel->flash();
	wheel_value->draw(h, s, v);
	wheel_value->flash();
	output->draw();
	output->flash();
	hue->update((int)h);
	saturation->update(s);
	value->update(v);

	ColorSpaces::hsv_to_rgb(r, g, b, h, s, v);
	red->update(r);
	green->update(g);
	blue->update(b);
	if(thread->do_alpha)
		alpha->update(a);
}

int ColorWindow::handle_event()
{
	float r, g, b;

	ColorSpaces::hsv_to_rgb(r, g, b, h, s, v);
	if(!thread->handle_new_color(r * 0xffff, g * 0xffff, b * 0xffff, a * 0xffff))
	{
		int result = (((int)(r * 255)) << 16) | (((int)(g * 255)) << 8) | ((int)(b * 255));
		thread->handle_new_color(result, (int)(a * 255));
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
	float h;
	float s;
	float v = 1;
	float r, g, b;
	float x1, y1, x2, y2;
	float distance;
	int default_r, default_g, default_b;
	VFrame frame(0, get_w(), get_h(), BC_RGBA8888);

	x1 = get_w() / 2;
	y1 = get_h() / 2;
	default_r = (get_resources()->get_bg_color() & 0xff0000) >> 16;
	default_g = (get_resources()->get_bg_color() & 0xff00) >> 8;
	default_b = (get_resources()->get_bg_color() & 0xff);

	for(y2 = 0; y2 < get_h(); y2++)
	{
		unsigned char *row = frame.get_row_ptr((int)y2);

		for(x2 = 0; x2 < get_w(); x2++)
		{
			distance = sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
			if(distance > x1)
			{
				row[(int)x2 * 4] = default_r;
				row[(int)x2 * 4 + 1] = default_g;
				row[(int)x2 * 4 + 2] = default_b;
				row[(int)x2 * 4 + 3] = 0;
			}
			else
			{
				h = get_angle(x1, y1, x2, y2);
				s = distance / x1;
				ColorSpaces::hsv_to_rgb(r, g, b, h, s, v);
				row[(int)x2 * 4] = (int)(r * 255);
				row[(int)x2 * 4 + 1] = (int)(g * 255);
				row[(int)x2 * 4 + 2] = (int)(b * 255);
				row[(int)x2 * 4 + 3] = 255;
			}
		}
	}

	draw_vframe(&frame,
		0, 0, get_w(), get_h(),
		0, 0, get_w(), get_h(), 0);

	oldhue = window->h;
	oldsaturation = window->s;
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

		window->h = get_angle(get_w() / 2,
			get_h() / 2,
			cursor_x,
			cursor_y);
		x1 = get_w() / 2 - cursor_x;
		y1 = get_h() / 2 - cursor_y;
		distance = round(sqrt(x1 * x1 + y1 * y1));
		if(distance > get_w() / 2)
			distance = get_w() / 2;
		window->s = (float)distance / (get_w() / 2);
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

void PaletteWheel::draw(float hue, float saturation)
{
	int x, y, w, h;

	w = get_w() / 2;
	h = get_h() / 2;

	if(hue > 0 && hue < 90)
	{
		x = (int)(w + w * cos(torads(90 - hue)) * saturation);
		y = (int)(h - h * sin(torads(90 - hue)) * saturation);
	}
	else
	if(hue > 90 && hue < 180)
	{
		x = (int)(w + w * cos(torads(hue - 90)) * saturation);
		y = (int)(h + h * sin(torads(hue - 90)) * saturation);
	}
	else
	if(hue > 180 && hue < 270)
	{
		x = (int)(w - w * cos(torads(270 - hue)) * saturation);
		y = (int)(h + h * sin(torads(270 - hue)) * saturation);
	}
	else
	if(hue > 270 && hue < 360)
	{
		x = (int)(w - w * cos(torads(hue - 270)) * saturation);
		y = (int)(h - w * sin(torads(hue - 270)) * saturation);
	}
	else
	if(hue == 0) 
	{
		x = w;
		y = (int)(h - h * saturation);
	}
	else
	if(hue == 90)
	{
		x = (int)(w + w * saturation);
		y = h;
	}
	else
	if(hue == 180)
	{
		x = w;
		y = (int)(h + h * saturation);
	}
	else
	if(hue == 270)
	{
		x = (int)(w - w * saturation);
		y = h;
	}

	set_inverse();
	set_color(WHITE);
	draw_circle(x - 5, y - 5, 10, 10);
	set_opaque();
}

int PaletteWheel::get_angle(float x1, float y1, float x2, float y2)
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
		window->v = (float)(get_h() - get_cursor_y()) / get_h();
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

void PaletteWheelValue::draw(float hue, float saturation, float value)
{
	float r_f, g_f, b_f;
	int i, j, r, g, b;
	for(i = get_h() - 1; i >= 0; i--)
	{
		unsigned char *row = frame->get_row_ptr(i);

		ColorSpaces::hsv_to_rgb(r_f, g_f, b_f, hue, saturation, (float)(get_h() - 1 - i) / get_h());
		r = (int)(r_f * 255);
		g = (int)(g_f * 255);
		b = (int)(b_f * 255);
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
	float r_f, g_f, b_f;

	ColorSpaces::hsv_to_rgb(r_f, g_f, b_f, window->h, window->s, window->v);
	set_color(((int)(r_f * 255) << 16) | ((int)(g_f * 255) << 8) | ((int)(b_f * 255)));
	draw_box(0, 0, get_w(), get_h());
}


PaletteHue::PaletteHue(ColorWindow *window, int x, int y)
 : BC_ISlider(x, y, 0, 150, 200, 0, 359, (int)(window->h), 0)
{
	this->window = window;
}

int PaletteHue::handle_event()
{
	window->h = get_value();
	window->update_display();
	window->handle_event();
	return 1;
}


PaletteSaturation::PaletteSaturation(ColorWindow *window, int x, int y)
 : BC_FSlider(x, y, 0, 150, 200, 0, 1.0, window->s, 0)
{
	this->window = window;
	set_precision(0.01);
}

int PaletteSaturation::handle_event()
{
	window->s = get_value();
	window->update_display();
	window->handle_event();
	return 1;
}


PaletteValue::PaletteValue(ColorWindow *window, int x, int y)
 : BC_FSlider(x, y, 0, 150, 200, 0, 1.0, window->v, 0)
{
	this->window = window;
	set_precision(0.01);
}

int PaletteValue::handle_event()
{
	window->v = get_value();
	window->update_display();
	window->handle_event();
	return 1;
}


PaletteRed::PaletteRed(ColorWindow *window, int x, int y)
 : BC_FSlider(x, y, 0, 150, 200, 0, 1, window->r, 0)
{
	this->window = window;
	set_precision(0.01);
}

int PaletteRed::handle_event()
{
	window->update_rgb();
	window->handle_event();
	return 1;
}


PaletteGreen::PaletteGreen(ColorWindow *window, int x, int y)
 : BC_FSlider(x, y, 0, 150, 200, 0, 1, window->g, 0)
{
	this->window = window;
	set_precision(0.01);
}

int PaletteGreen::handle_event()
{
	window->update_rgb();
	window->handle_event();
	return 1;
}


PaletteBlue::PaletteBlue(ColorWindow *window, int x, int y)
 : BC_FSlider(x, y, 0, 150, 200, 0, 1, window->b, 0)
{
	this->window = window;
	set_precision(0.01);
}

int PaletteBlue::handle_event()
{
	window->update_rgb();
	window->handle_event();
	return 1;
}


PaletteAlpha::PaletteAlpha(ColorWindow *window, int x, int y)
 : BC_FSlider(x, y, 0, 150, 200, 0, 1, window->a, 0)
{
	this->window = window;
	set_precision(0.01);
}

int PaletteAlpha::handle_event()
{
	window->a = get_value();
	window->handle_event();
	return 1;
}
