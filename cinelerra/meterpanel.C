
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

#include "edl.h"
#include "edlsession.h"
#include "language.h"
#include "meterpanel.h"
#include "mwindow.h"
#include "preferences.h"
#include "theme.h"
#include "vframe.h"



MeterPanel::MeterPanel(MWindow *mwindow, 
	BC_WindowBase *subwindow, 
	int x, 
	int y,
	int h,
	int meter_count,
	int use_meters,
	int use_recording)
{
	this->subwindow = subwindow;
	this->mwindow = mwindow;
	this->x = x;
	this->y = y;
	this->h = h;
	this->meter_count = meter_count;
	this->use_meters = use_meters;
	this->use_recording = use_recording;
}


MeterPanel::~MeterPanel()
{
	meters.remove_all_objects();
}

int MeterPanel::get_meters_width(int meter_count, int use_meters)
{
//printf("MeterPanel::get_meters_width %d %d\n", BC_Meter::get_title_w(), BC_Meter::get_meter_w());
	return use_meters ? 
		(BC_Meter::get_title_w() + BC_Meter::get_meter_w() * meter_count) : 
		0;
}

void MeterPanel::reposition_window(int x, int y, int h)
{
	this->x = x;
	this->y = y;
	this->h = h;

//	reset->reposition_window(get_reset_x(), get_reset_y());

//printf("MeterPanel::reposition_window 0 %d\n", meter_count);

	for(int i = 0; i < meter_count; i++)
	{
//printf("MeterPanel::reposition_window 1 %d\n", x);
		meters.values[i]->reposition_window(x, y, get_meter_h());
		x += get_meter_w(i);
	}
}

int MeterPanel::change_status_event()
{
//printf("MeterPanel::change_status_event\n");
	return 1;
}

int MeterPanel::get_reset_x()
{
	return x + 
		get_meters_width(meter_count, use_meters) - 
		mwindow->theme->over_button[0]->get_w();
}

int MeterPanel::get_reset_y()
{
	return y + h - mwindow->theme->over_button[0]->get_h();
}

int MeterPanel::get_meter_w(int number)
{
	return (number == 0) ? BC_Meter::get_title_w() + BC_Meter::get_meter_w() : BC_Meter::get_meter_w();
}

int MeterPanel::get_meter_h()
{
	return /* reset->get_y() - this->y - */ this->h - 5;
}

void MeterPanel::update(double *levels)
{
	if(subwindow->get_hidden()) return;


	for(int i = 0; 
		i < meter_count; 
		i++)
	{
		meters.values[i]->update(levels[i], levels[i] > 1);
	}
}

void MeterPanel::stop_meters()
{
	for(int i = 0; 
		i < meter_count; 
		i++)
	{
		meters.values[i]->reset();
	}
}


int MeterPanel::create_objects()
{
	set_meters(meter_count, use_meters);
	return 0;
}

int MeterPanel::set_meters(int meter_count, int use_meters)
{
	if(meter_count != meters.total || use_meters != this->use_meters)
	{
// Delete old meters
		meters.remove_all_objects();

		this->meter_count = meter_count;
		this->use_meters = use_meters;
//		if(!use_meters) this->meter_count = 0;

		if(meter_count)
		{
			int x = this->x;
			int y = this->y;
			int h = get_meter_h();
			for(int i = 0; i < meter_count; i++)
			{
				MeterMeter *new_meter;
				subwindow->add_subwindow(new_meter = new MeterMeter(mwindow,
						this,
						x, 
						y, 
						h, 
						(i == 0)));
				meters.append(new_meter);
        		x += get_meter_w(i);
			}
		}
	}

	return 0;
}

void MeterPanel::reset_meters()
{
	for(int i = 0; i < meters.total; i++)
		meters.values[i]->reset_over();
}


void MeterPanel::change_format(int mode, int min, int max)
{
	for(int i = 0; i < meters.total; i++)
	{
		if(use_recording)
			meters.values[i]->change_format(mode, min, 0);
		else
			meters.values[i]->change_format(mode, min, max);
	}
}









MeterReset::MeterReset(MWindow *mwindow, MeterPanel *panel, int x, int y)
 : BC_Button(x, y, mwindow->theme->over_button)
{
	this->mwindow = mwindow;
	this->panel = panel;
}

MeterReset::~MeterReset()
{
}
	
int MeterReset::handle_event()
{
	for(int i = 0; i < panel->meters.total; i++)
		panel->meters.values[i]->reset_over();
	return 1;
}





MeterMeter::MeterMeter(MWindow *mwindow, 
	MeterPanel *panel, 
	int x, 
	int y, 
	int h, 
	int titles)
 : BC_Meter(x,
 	y,
	METER_VERT,
	h,
	mwindow->edl->session->min_meter_db, 
	panel->use_recording ? 0 : mwindow->edl->session->max_meter_db, 
	mwindow->edl->session->meter_format,
	titles,
	TRACKING_RATE * 10,
	TRACKING_RATE)
{
	this->mwindow = mwindow;
	this->panel = panel;
}

MeterMeter::~MeterMeter()
{
}


int MeterMeter::button_press_event()
{
	if(is_event_win() && BC_WindowBase::cursor_inside())
	{
		panel->reset_meters();
		mwindow->reset_meters();
		return 1;
	}

	return 0;
}





MeterShow::MeterShow(MWindow *mwindow, MeterPanel *panel, int x, int y)
 : BC_Toggle(x, 
 		y, 
		mwindow->theme->get_image_set("meters"), 
		panel->use_meters)
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_tooltip(_("Show meters"));
}


MeterShow::~MeterShow()
{
}


int MeterShow::handle_event()
{
//printf("MeterShow::MeterShow 1 %d\n",panel->use_meters );
	panel->use_meters = get_value();
	panel->change_status_event();
	return 1;
}
