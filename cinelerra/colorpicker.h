// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef COLORPICKER_H
#define COLORPICKER_H

#include "bcsubwindow.h"
#include "bcslider.h"
#include "bcwindow.h"
#include "condition.inc"
#include "mutex.inc"
#include "thread.h"
#include "vframe.inc"
#include "thread.h"

class ColorWindow;
class PaletteWheel;
class PaletteWheelValue;
class PaletteOutput;
class PaletteHue;
class PaletteFSlider;
class PaletteISlider;

class ColorThread : public Thread
{
public:
	ColorThread(int do_alpha = 0, const char *title = 0, VFrame* icon = 0);
	~ColorThread();

	void run();
	void start_window(int r, int g, int b, int a);
	void start_window(int output, int alpha);
	virtual int handle_new_color(int output, int alpha) { return 0; }
	virtual int handle_new_color(int red, int green,
		int blue, int alpha) { return 0; };
	void update_gui(int r, int g, int b, int a);
	void update_gui(int output, int a);

private:
	friend class ColorWindow;

	ColorWindow *window;
	Condition *completion;
// protects window, output, alpha
	Mutex *mutex;
	int do_alpha;
// u16 colors
	int red;
	int green;
	int blue;
	int alpha;
	const char *title;
	VFrame *icon;
};

class ColorWindow : public BC_Window
{
public:
	ColorWindow(ColorThread *thread, int x, int y, const char *title);

	void change_values();
	void close_event();
	void update_display();
	void update_rgb();
	int handle_event();

	ColorThread *thread;

	PaletteWheel *wheel;
	PaletteWheelValue *wheel_value;
	PaletteOutput *output;
	PaletteHue *hue;
	PaletteFSlider *saturation;
	PaletteFSlider *value;
	PaletteISlider *red;
	PaletteISlider *green;
	PaletteISlider *blue;
	PaletteISlider *alpha;
	VFrame *value_bitmap;
	int hueval;
	double sat;
	double val;
};


class PaletteWheel : public BC_SubWindow
{
public:
	PaletteWheel(ColorWindow *window, int x, int y);

	void show();
	int button_press_event();
	int cursor_motion_event();
	int button_release_event();

	void draw(int hue, double saturation);
	int get_angle(int x1, int y1, int x2, int y2);
	double torads(double angle);

	ColorWindow *window;
	float oldhue;
	float oldsaturation;
	int button_down;
};

class PaletteWheelValue : public BC_SubWindow
{
public:
	PaletteWheelValue(ColorWindow *window, int x, int y);
	~PaletteWheelValue();

	int button_press_event();
	int cursor_motion_event();
	int button_release_event();
	void draw(int hue, double saturation, double value);

	ColorWindow *window;
	int button_down;
// Gradient
	VFrame *frame;
};

class PaletteOutput : public BC_SubWindow
{
public:
	PaletteOutput(ColorWindow *window, int x, int y);

	int handle_event();
	void draw();

	ColorWindow *window;
};

class PaletteHue : public BC_ISlider
{
public:
	PaletteHue(ColorWindow *window, int x, int y);

	int handle_event();

	ColorWindow *window;
};

class PaletteFSlider : public BC_FSlider
{
public:
	PaletteFSlider(ColorWindow *window, int x, int y, double *value);

	int handle_event();

	ColorWindow *window;
	double *value;
};

class PaletteISlider : public BC_ISlider
{
public:
	PaletteISlider(ColorWindow *window, int x, int y, int *value);

	int handle_event();

	ColorWindow *window;
};

#endif
