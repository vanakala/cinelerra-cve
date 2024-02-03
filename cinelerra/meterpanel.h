// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef METERPANEL_H
#define METERPANEL_H

#include "bcbutton.h"
#include "bctoggle.h"
#include "bcmeter.h"

class MeterReset;
class MeterMeter;


class MeterPanel
{
public:
	MeterPanel(BC_WindowBase *subwindow, int x, int y, int h,
		int meter_count, int use_meters,
		int use_recording = 0);
	~MeterPanel();

	void set_meters(int meter_count, int use_meters);
	static int get_meters_width(int meter_count, int use_meters);
	void reposition_window(int x, int y, int h);
	int get_reset_x();
	int get_reset_y();
	int get_meter_h();
	int get_meter_w(int number);
	void update(double *levels);
	void stop_meters();
	void change_format(int min, int max);
	virtual int change_status_event() { return 0; };
	void reset_meters();
	void set_delays(int over_delay, int peak_delay);

	BC_WindowBase *subwindow;
	ArrayList<MeterMeter*> meters;
	MeterReset *reset;
	int meter_count;
	int use_meters;
	int x, y, h;
	int use_recording;
};


class MeterReset : public BC_Button
{
public:
	MeterReset(MeterPanel *panel, int x, int y);

	int handle_event();

	MeterPanel *panel;
};


class MeterShow : public BC_Toggle
{
public:
	MeterShow(MeterPanel *panel, int x, int y);

	int handle_event();

	MeterPanel *panel;
};

class MeterMeter : public BC_Meter
{
public:
	MeterMeter(MeterPanel *panel, int x, int y, int h, int titles);

	int button_press_event();

	MeterPanel *panel;
};

#endif
