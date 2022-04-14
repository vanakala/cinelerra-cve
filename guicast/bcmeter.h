// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef BCMETER_H
#define BCMETER_H

#include "bcmeter.inc"
#include "bcsubwindow.h"
#include "units.h"

#define METER_TYPES 2

#define METER_DB 0
#define METER_INT 1
#define METER_VERT 0
#define METER_HORIZ 1

// Distance from subwindow border to top and bottom tick mark
#define METER_MARGIN 0

class BC_Meter : public BC_SubWindow
{
public:
	BC_Meter(int x, 
		int y, 
		int orientation, 
		int pixels, 
		int min, /* = -40, */
		int max,
		int use_titles); /* = 0, */
	virtual ~BC_Meter();

	void initialize();
	void set_images(VFrame **data);

// over_delay - number of updates before over dissappears
// peak_delay - number of updates before peak updates
	void set_delays(int over_delay, int peak_delay);
	int region_pixel(int region);
	int region_pixels(int region);
	virtual int button_press_event();

	static int get_title_w();
	static int get_meter_w();
	void update(double new_value, int over);
	void reposition_window(int x, int y, int pixels);
	void reset();
	void reset_over();
	void change_format(int min, int max);

private:
	void draw_titles();
	void draw_face();
	int level_to_pixel(double level);
	void get_divisions();

	BC_Pixmap *images[TOTAL_METER_IMAGES];
	int orientation;
// Number of pixels in the longest dimension
	int pixels;
	int low_division;
	int medium_division;
	int high_division;
	int use_titles;
// Tick mark positions
	ArrayList<int> tick_pixels;
// Title positions
	ArrayList<int> title_pixels;
	ArrayList<char*> db_titles;
	double level;
	double peak;
	DB db;
	int peak_timer;

	int peak_pixel, level_pixel, peak_pixel1, peak_pixel2;
	int over_count, over_timer;
	int min;
	int max;
	int over_delay;       // Number of updates the over warning lasts.
	int peak_delay;       // Number of updates the peak lasts.
};

#endif
