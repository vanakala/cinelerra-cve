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
class PaletteSaturation;
class PaletteValue;
class PaletteRed;
class PaletteGreen;
class PaletteBlue;
class PaletteAlpha;

class ColorThread : public Thread
{
public:
	ColorThread(int do_alpha = 0, const char *title = 0, VFrame* icon = 0);
	~ColorThread();

	void run();
	void start_window(int r, int g, int b, int alpha);
	void start_window(int output, int alpha);
	virtual int handle_new_color(int output, int alpha) { return 0; }
	virtual int handle_new_color(int red, int green,
		int blue, int alpha) { return 0; };
	void update_gui(int r, int g, int b, int alpha);
	void update_gui(int output, int alpha);

private:
	friend class ColorWindow;

	ColorWindow *window;
	Condition *completion;
// protects window, output, alpha
	Mutex *mutex;
// Starting color
	int output;
	int alpha;
	int do_alpha;
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
	PaletteSaturation *saturation;
	PaletteValue *value;
	PaletteRed *red;
	PaletteGreen *green;
	PaletteBlue *blue;
	PaletteAlpha *alpha;
	VFrame *value_bitmap;
	float h, s, v, r, g, b, a;
};


class PaletteWheel : public BC_SubWindow
{
public:
	PaletteWheel(ColorWindow *window, int x, int y);

	void show();
	int button_press_event();
	int cursor_motion_event();
	int button_release_event();

	void draw(float hue, float saturation);
	int get_angle(float x1, float y1, float x2, float y2);
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
	void draw(float hue, float saturation, float value);

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

class PaletteSaturation : public BC_FSlider
{
public:
	PaletteSaturation(ColorWindow *window, int x, int y);

	int handle_event();
	ColorWindow *window;
};

class PaletteValue : public BC_FSlider
{
public:
	PaletteValue(ColorWindow *window, int x, int y);

	int handle_event();

	ColorWindow *window;
};

class PaletteRed : public BC_FSlider
{
public:
	PaletteRed(ColorWindow *window, int x, int y);

	int handle_event();

	ColorWindow *window;
};

class PaletteGreen : public BC_FSlider
{
public:
	PaletteGreen(ColorWindow *window, int x, int y);

	int handle_event();

	ColorWindow *window;
};

class PaletteBlue : public BC_FSlider
{
public:
	PaletteBlue(ColorWindow *window, int x, int y);

	int handle_event();

	ColorWindow *window;
};

class PaletteAlpha : public BC_FSlider
{
public:
	PaletteAlpha(ColorWindow *window, int x, int y);

	int handle_event();

	ColorWindow *window;
};

#endif
