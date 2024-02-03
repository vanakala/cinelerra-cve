// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bcsignals.h"
#include "edl.h"
#include "edlsession.h"
#include "language.h"
#include "meterpanel.h"
#include "mwindow.h"
#include "preferences.h"
#include "theme.h"
#include "vframe.h"


MeterPanel::MeterPanel(BC_WindowBase *subwindow, int x, int y, int h,
	int meter_count, int use_meters, int use_recording)
{
	this->subwindow = subwindow;
	this->x = x;
	this->y = y;
	this->h = h;
	this->meter_count = meter_count;
	this->use_meters = use_meters;
	this->use_recording = use_recording;
	set_meters(meter_count, use_meters);
}

MeterPanel::~MeterPanel()
{
	meters.remove_all_objects();
}

int MeterPanel::get_meters_width(int meter_count, int use_meters)
{
	return use_meters ?
		(BC_Meter::get_title_w() + BC_Meter::get_meter_w() * meter_count) : 0;
}

void MeterPanel::reposition_window(int x, int y, int h)
{
	this->x = x;
	this->y = y;
	this->h = h;

	for(int i = 0; i < meter_count; i++)
	{
		meters.values[i]->reposition_window(x, y, get_meter_h());
		x += get_meter_w(i);
	}
}

int MeterPanel::get_reset_x()
{
	return x + get_meters_width(meter_count, use_meters) -
		theme_global->over_button[0]->get_w();
}

int MeterPanel::get_reset_y()
{
	return y + h - theme_global->over_button[0]->get_h();
}

int MeterPanel::get_meter_w(int number)
{
	return (number == 0) ? BC_Meter::get_title_w() + BC_Meter::get_meter_w() : BC_Meter::get_meter_w();
}

int MeterPanel::get_meter_h()
{
	return this->h - 5;
}

void MeterPanel::update(double *levels)
{
	if(subwindow->get_hidden())
		return;

	for(int i = 0; i < meter_count; i++)
		meters.values[i]->update(levels[i], levels[i] > 1);
}

void MeterPanel::stop_meters()
{
	for(int i = 0; i < meter_count; i++)
		meters.values[i]->reset();
}

void MeterPanel::set_meters(int meter_count, int use_meters)
{
	if(meter_count != meters.total || use_meters != this->use_meters)
	{
// Delete old meters
		meters.remove_all_objects();

		this->meter_count = meter_count;
		this->use_meters = use_meters;

		if(meter_count)
		{
			int x = this->x;
			int y = this->y;
			int h = get_meter_h();

			for(int i = 0; i < meter_count; i++)
			{
				MeterMeter *new_meter;
				subwindow->add_subwindow(new_meter = new MeterMeter(this,
					x, y, h, (i == 0)));
				meters.append(new_meter);
				x += get_meter_w(i);
			}
		}
	}
}

void MeterPanel::reset_meters()
{
	for(int i = 0; i < meters.total; i++)
		meters.values[i]->reset_over();
}

void MeterPanel::set_delays(int over_delay, int peak_delay)
{
	for(int i = 0; i < meters.total; i++)
		meters.values[i]->set_delays(over_delay, peak_delay);
}

void MeterPanel::change_format(int min, int max)
{
	for(int i = 0; i < meters.total; i++)
	{
		if(use_recording)
			meters.values[i]->change_format(min, 0);
		else
			meters.values[i]->change_format(min, max);
	}
}


MeterReset::MeterReset(MeterPanel *panel, int x, int y)
 : BC_Button(x, y, theme_global->over_button)
{
	this->panel = panel;
}

int MeterReset::handle_event()
{
	for(int i = 0; i < panel->meters.total; i++)
		panel->meters.values[i]->reset_over();
	return 1;
}


MeterMeter::MeterMeter(MeterPanel *panel, int x, int y, int h, int titles)
 : BC_Meter(x, y, METER_VERT, h, edlsession->min_meter_db,
	panel->use_recording ? 0 : edlsession->max_meter_db,
	titles)
{
	this->panel = panel;
}

int MeterMeter::button_press_event()
{
	if(is_event_win() && BC_WindowBase::cursor_inside())
	{
		panel->reset_meters();
		mwindow_global->reset_meters();
		return 1;
	}
	return 0;
}


MeterShow::MeterShow(MeterPanel *panel, int x, int y)
 : BC_Toggle(x, y, theme_global->get_image_set("meters"),
	panel->use_meters)
{
	this->panel = panel;
	set_tooltip(_("Show meters"));
}

int MeterShow::handle_event()
{
	panel->use_meters = get_value();
	return panel->change_status_event();
}
