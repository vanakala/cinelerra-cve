// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef BCPOT_H
#define BCPOT_H

#include "bcpixmap.h"
#include "vframe.inc"
#include "bcsubwindow.h"

#define POT_UP 0
#define POT_HIGH 1
#define POT_DN 2
#define POT_STATES 3

class BC_FPot;
class BC_IPot;
class BC_QPot;
class BC_PercentagePot;

class BC_Pot : public BC_SubWindow
{
public:
	BC_Pot(int x, int y, VFrame **data);
	virtual ~BC_Pot() {};

	friend class BC_FPot;
	friend class BC_IPot;
	friend class BC_QPot;
	friend class BC_PercentagePot;

	static int calculate_h();
	void initialize();
	virtual double get_percentage() { return 0; };
	virtual int percentage_to_value(double percentage) { return 0; };
	virtual int handle_event() { return 0; };
	virtual const char* get_caption() { return ""; };
	virtual void increase_value() {};
	virtual void decrease_value() {};
	void set_use_caption(int value);

	void reposition_window(int x, int y);
	void repeat_event(int repeat_id);
	int cursor_enter_event();
	void cursor_leave_event();
	int button_press_event();
	virtual int button_release_event();
	int cursor_motion_event();
	int keypress_event();

private:
	void set_data(VFrame **data);
	void draw();
	double percentage_to_angle(double percentage);
	double angle_to_percentage(double angle);
	void angle_to_coords(int &x1, int &y1, int &x2, int &y2, double angle);
	double coords_to_angle(int x2, int y2);
	void show_value_tooltip();

	VFrame **data;
	BC_Pixmap *images[POT_STATES];
	char caption[BCTEXTLEN], temp_tooltip_text[BCTEXTLEN];
	int status;
	int keypress_tooltip_timer;
	double angle_offset;
	double start_cursor_angle;
	double start_needle_angle;
	double prev_angle, angle_correction;
	int use_caption;
};

class BC_FPot : public BC_Pot
{
public:
	BC_FPot(int x, 
		int y, 
		double value,
		double minvalue,
		double maxvalue,
		VFrame **data = 0);

	const char* get_caption();
	void increase_value();
	void decrease_value();
	double get_percentage();
	double get_value();
	int percentage_to_value(double percentage);
	void update(double value);
	void update(double value, double minvalue, double maxvalue);
	void set_precision(double value);

private:
	double value, minvalue, maxvalue;
	double precision;
};

class BC_IPot : public BC_Pot
{
public:
	BC_IPot(int x, 
		int y, 
		int value,
		int minvalue,
		int maxvalue,
		VFrame **data = 0);

	const char* get_caption();
	void increase_value();
	void decrease_value();
	double get_percentage();
	int percentage_to_value(double percentage);
	int get_value();
	void update(int value);
	void update(int value, int minvalue, int maxvalue);

private:
	int value, minvalue, maxvalue;
};

class BC_QPot : public BC_Pot
{
public:
	BC_QPot(int x, 
		int y, 
		int value,      // Units of frequencies
		VFrame **data = 0);

	const char* get_caption();
	void increase_value();
	void decrease_value();
	double get_percentage();
	int percentage_to_value(double percentage);
// Units of frequencies
	int get_value();
// Units of frequencies
	void update(int value);

private:
// Units of frequency index
	int value, minvalue, maxvalue;
};

class BC_PercentagePot : public BC_Pot
{
public:
	BC_PercentagePot(int x, 
		int y, 
		double value,
		double minvalue,
		double maxvalue,
		VFrame **data = 0);

	const char* get_caption();
	void increase_value();
	void decrease_value();
	double get_percentage();
	double get_value();
	int percentage_to_value(double percentage);
	void update(double value);

private:
	double value, minvalue, maxvalue;
};

#endif
