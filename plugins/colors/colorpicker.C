
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

#include "bcdisplayinfo.h"
#include "colorpicker.h"
#include "condition.h"
#include "language.h"
#include "mutex.h"
#include "mwindow.inc"
#include "plugincolors.h"
#include "vframe.h"

#include <string.h>
#include <unistd.h>


ColorThread::ColorThread(int do_alpha, char *title)
 : Thread()
{
	window = 0;
	this->title = title;
	this->do_alpha = do_alpha;
	set_synchronous(0);
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
	delete mutex;
	delete completion;
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
	BC_DisplayInfo info;
//printf("ColorThread::run 1\n");
	char window_title[BCTEXTLEN];

	strcpy(window_title, PROGRAM_NAME ": ");
	if(title)
		strcat(window_title, title);
	else
		strcat(window_title, _("Color Picker"));


	mutex->lock("ColorThread::run 1");
	window = new ColorWindow(this, 
		info.get_abs_cursor_x() - 200, 
		info.get_abs_cursor_y() - 200,
		window_title);
	window->create_objects();
	mutex->unlock();
	window->run_window();
	mutex->lock("lorThread::run 2");
	delete window;
	window = 0;
	mutex->unlock();
	completion->unlock();
}

void ColorThread::update_gui(int output, int alpha)
{
	mutex->lock("ColorThread::update_gui");
	if (window)
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

int ColorThread::handle_new_color(int output, int alpha)
{
	printf("ColorThread::handle_new_color undefined.\n");
	return 0;
}





ColorWindow::ColorWindow(ColorThread *thread, int x, int y, char *title)
 : BC_Window(title, 
	x,
	y,
	410, 
	320, 
	410, 
	320, 
	0, 
	0,
	1)
{
	this->thread = thread;
}

void ColorWindow::create_objects()
{
	int x = 10, init_x = 10, y = 10, init_y = 10;
	change_values();
	
	
	
//printf("ColorWindow::create_objects 1 %p\n", this);
	add_tool(wheel = new PaletteWheel(this, x, y));
//printf("ColorWindow::create_objects 1\n");
	wheel->create_objects();
//printf("ColorWindow::create_objects 1\n");

	x += 180;
	add_tool(wheel_value = new PaletteWheelValue(this, x, y));
//printf("ColorWindow::create_objects 1\n");
	wheel_value->create_objects();


	y += 180;
	x = init_x; 
//printf("ColorWindow::create_objects 1\n");
	add_tool(output = new PaletteOutput(this, x, y));
//printf("ColorWindow::create_objects 1\n");
	output->create_objects();
//printf("ColorWindow::create_objects 1\n");
	
	x += 240; y = init_y;
	add_tool(new BC_Title(x, y, _("Hue"), SMALLFONT));
	y += 15;
//printf("ColorWindow::create_objects 1 %p\n", this);
	add_tool(hue = new PaletteHue(this, x, y));
	y += 30;
//printf("ColorWindow::create_objects 1\n");
	add_tool(new BC_Title(x, y, _("Saturation"), SMALLFONT));
	y += 15;
//printf("ColorWindow::create_objects 1\n");
	add_tool(saturation = new PaletteSaturation(this, x, y));
	y += 30;
//printf("ColorWindow::create_objects 1\n");
	add_tool(new BC_Title(x, y, _("Value"), SMALLFONT));
	y += 15;
//printf("ColorWindow::create_objects 1\n");
	add_tool(value = new PaletteValue(this, x, y));
	y += 30;
//printf("ColorWindow::create_objects 1\n");
	add_tool(new BC_Title(x, y, _("Red"), SMALLFONT));
	y += 15;
//printf("ColorWindow::create_objects 1\n");
	add_tool(red = new PaletteRed(this, x, y));
	y += 30;
//printf("ColorWindow::create_objects 1\n");
	add_tool(new BC_Title(x, y, _("Green"), SMALLFONT));
	y += 15;
//printf("ColorWindow::create_objects 1\n");
	add_tool(green = new PaletteGreen(this, x, y));
	y += 30;
//printf("ColorWindow::create_objects 1\n");
	add_tool(new BC_Title(x, y, _("Blue"), SMALLFONT));
	y += 15;
//printf("ColorWindow::create_objects 1\n");
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
	return;
}


void ColorWindow::change_values()
 {
 	r = (float)((thread->output & 0xff0000) >> 16) / 255;
 	g = (float)((thread->output & 0xff00) >> 8) / 255;
 	b = (float)((thread->output & 0xff)) / 255;
 	HSV::rgb_to_hsv(r, g, b, h, s, v);
 	a = (float)thread->alpha / 255;
}


int ColorWindow::close_event()
{
	set_done(0);
	return 1;
}


void ColorWindow::update_rgb()
{
	HSV::rgb_to_hsv(red->get_value(), 
				green->get_value(), 
				blue->get_value(), 
				h, 
				s, 
				v);
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

	wheel->draw(wheel->oldhue, 
				wheel->oldsaturation);
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

	HSV::hsv_to_rgb(r, g, b, h, s, v);
	red->update(r);
	green->update(g);
	blue->update(b);
	if(thread->do_alpha)
	{
		alpha->update(a);
	}
}

int ColorWindow::handle_event()
{
	float r, g, b;
	HSV::hsv_to_rgb(r, g, b, h, s, v);
	int result = (((int)(r * 255)) << 16) | (((int)(g * 255)) << 8) | ((int)(b * 255));
	thread->handle_new_color(result, (int)(a * 255));
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
PaletteWheel::~PaletteWheel()
{
}

int PaletteWheel::button_press_event()
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

int PaletteWheel::cursor_motion_event()
{
	int x1, y1, distance;
	if(button_down && is_event_win())
	{
		window->h = get_angle(get_w() / 2, 
			get_h() / 2, 
			get_cursor_x(), 
			get_cursor_y());
		x1 = get_w() / 2 - get_cursor_x();
		y1 = get_h() / 2 - get_cursor_y();
		distance = (int)sqrt(x1 * x1 + y1 * y1);
		if(distance > get_w() / 2) distance = get_w() / 2;
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

int PaletteWheel::create_objects()
{
// Upper right
//printf("PaletteWheel::create_objects 1\n");
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
//printf("PaletteWheel::create_objects 1\n");

	for(y2 = 0; y2 < get_h(); y2++)
	{
		for(x2 = 0; x2 < get_w(); x2++)
		{
			distance = sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
			if(distance > x1)
			{
				((unsigned char*)frame.get_rows()[(int)y2])[(int)x2 * 4] = default_r;
				((unsigned char*)frame.get_rows()[(int)y2])[(int)x2 * 4 + 1] = default_g;
				((unsigned char*)frame.get_rows()[(int)y2])[(int)x2 * 4 + 2] = default_b;
				((unsigned char*)frame.get_rows()[(int)y2])[(int)x2 * 4 + 3] = 0;
			}
			else
			{
				h = get_angle(x1, y1, x2, y2);
				s = distance / x1;
				HSV::hsv_to_rgb(r, g, b, h, s, v);
				((unsigned char*)frame.get_rows()[(int)y2])[(int)x2 * 4] = (int)(r * 255);
				((unsigned char*)frame.get_rows()[(int)y2])[(int)x2 * 4 + 1] = (int)(g * 255);
				((unsigned char*)frame.get_rows()[(int)y2])[(int)x2 * 4 + 2] = (int)(b * 255);
				((unsigned char*)frame.get_rows()[(int)y2])[(int)x2 * 4 + 3] = 255;
			}
		}
	}
//printf("PaletteWheel::create_objects 1\n");

	draw_vframe(&frame, 
		0, 
		0, 
		get_w(), 
		get_h(), 
		0, 
		0, 
		get_w(), 
		get_h(), 
		0);
//printf("PaletteWheel::create_objects 1\n");

	oldhue = window->h;
	oldsaturation = window->s;
//printf("PaletteWheel::create_objects 1\n");
	draw(oldhue, oldsaturation);
//printf("PaletteWheel::create_objects 1\n");
	flash();
//printf("PaletteWheel::create_objects 2\n");
	return 0;
}

float PaletteWheel::torads(float angle)
{
	return (float)angle / 360 * 2 * M_PI;
}


int PaletteWheel::draw(float hue, float saturation)
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
	return 0;
}

int PaletteWheel::get_angle(float x1, float y1, float x2, float y2)
{
	float result = atan2(x2 - x1, y1 - y2) * (360 / M_PI / 2);
	if (result < 0)
		result += 360;
	return (int)result;
}

PaletteWheelValue::PaletteWheelValue(ColorWindow *window, int x, int y)
 : BC_SubWindow(x, y, 40, 170, BLACK)
{
	this->window = window;
	button_down = 0;
}
PaletteWheelValue::~PaletteWheelValue()
{
	delete frame;
}

int PaletteWheelValue::create_objects()
{
	frame = new VFrame(0, get_w(), get_h(), BC_RGB888);
	draw(window->h, window->s, window->v);
	flash();
	return 0;
}

int PaletteWheelValue::button_press_event()
{
//printf("PaletteWheelValue::button_press 1 %d\n", is_event_win());
	if(get_cursor_x() >= 0 && get_cursor_x() < get_w() &&
		get_cursor_y() >= 0 && get_cursor_y() < get_h() && 
		is_event_win())
	{
//printf("PaletteWheelValue::button_press 2\n");
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
//printf("PaletteWheelValue::cursor_motion 1\n");
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
//printf("PaletteWheelValue::button_release 1\n");
		button_down = 0;
		return 1;
	}
	return 0;
}

int PaletteWheelValue::draw(float hue, float saturation, float value)
{
	float r_f, g_f, b_f;
	int i, j, r, g, b;
	for(i = get_h() - 1; i >= 0; i--)
	{
		HSV::hsv_to_rgb(r_f, g_f, b_f, hue, saturation, (float)(get_h() - 1 - i) / get_h());
		r = (int)(r_f * 255);
		g = (int)(g_f * 255);
		b = (int)(b_f * 255);
		for(j = 0; j < get_w(); j++)
		{
 			((unsigned char*)frame->get_rows()[i])[j * 3] = r;
 			((unsigned char*)frame->get_rows()[i])[j * 3 + 1] = g;
 			((unsigned char*)frame->get_rows()[i])[j * 3 + 2] = b;
		}
	}
	draw_vframe(frame, 
		0, 
		0, 
		get_w(), 
		get_h(), 
		0, 
		0, 
		get_w(), 
		get_h(), 
		0);
	set_color(BLACK);
	draw_line(0, 
		get_h() - (int)(value * get_h()), 
		get_w(), 
		get_h() - (int)(value * get_h()));
	return 0;
}

PaletteOutput::PaletteOutput(ColorWindow *window, int x, int y)
 : BC_SubWindow(x, y, 180, 30, BLACK)
{
	this->window = window;
}
PaletteOutput::~PaletteOutput()
{
}


int PaletteOutput::create_objects()
{
	draw();
	flash();
	return 0;
}

int PaletteOutput::handle_event()
{
	return 1;
}

int PaletteOutput::draw()
{
	float r_f, g_f, b_f;
	
	HSV::hsv_to_rgb(r_f, g_f, b_f, window->h, window->s, window->v);
	set_color(((int)(r_f * 255) << 16) | ((int)(g_f * 255) << 8) | ((int)(b_f * 255)));
	draw_box(0, 0, get_w(), get_h());
	return 0;
}

PaletteHue::PaletteHue(ColorWindow *window, int x, int y)
 : BC_ISlider(x, y, 0, 150, 200, 0, 359, (int)(window->h), 0)
{
	this->window = window;
}
PaletteHue::~PaletteHue()
{
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
PaletteSaturation::~PaletteSaturation()
{
}

int PaletteSaturation::handle_event()
{
//printf("PaletteSaturation::handle_event 1 %f\n", get_value());
	window->s = get_value();
	window->update_display();
//printf("PaletteSaturation::handle_event 2 %f\n", get_value());
	window->handle_event();
	return 1;
}

PaletteValue::PaletteValue(ColorWindow *window, int x, int y)
 : BC_FSlider(x, y, 0, 150, 200, 0, 1.0, window->v, 0)
{
	this->window = window;
	set_precision(0.01);
}
PaletteValue::~PaletteValue()
{
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
PaletteRed::~PaletteRed()
{
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
PaletteGreen::~PaletteGreen()
{
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
PaletteBlue::~PaletteBlue()
{
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
PaletteAlpha::~PaletteAlpha()
{
}

int PaletteAlpha::handle_event()
{
	window->a = get_value();
	window->handle_event();
	return 1;
}


