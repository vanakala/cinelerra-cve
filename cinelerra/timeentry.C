
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

#include "timeentry.h"
#include <string.h>

TimeEntry::TimeEntry(BC_WindowBase *gui, 
		int x, 
		int y, 
		int *output_day, 
		double *output_time,
		int time_format)
{
	this->gui = gui;
	this->x = x;
	this->y = y;
	this->output_day = output_day;
	this->output_time = output_time;
	this->time_format = time_format;
}

TimeEntry::~TimeEntry()
{
	if(output_day)
	{
		delete day_text;
		delete day_tumbler;
	}
	delete time_text;
}

char* TimeEntry::day_table[] = 
{
	"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "*"
};

int TimeEntry::day_to_int(char *day)
{
	for(int i = 0; i < TOTAL_DAYS; i++)
		if(!strcasecmp(day, day_table[i])) return i;
	return 0;
}


void TimeEntry::time_to_hours(char *result, double time)
{
	int hour = (int)(time / 3600);
	sprintf(result, "%d", hour);
}

void TimeEntry::time_to_minutes(char *result, double time)
{
	int hour = (int)(time / 3600);
	int minute = (int)(time / 60 - hour * 60);
	sprintf(result, "%d", minute);
}

void TimeEntry::time_to_seconds(char *result, double time)
{
	int hour = (int)(time / 3600);
	int minute = (int)(time / 60 - hour * 60);
	float second = (float)time - (int64_t)hour * 3600 - (int64_t)minute * 60;
	sprintf(result, "%.3f", second);
}

#define DEFAULT_TIMEW 200

void TimeEntry::create_objects()
{
	int x = this->x;
	int y = this->y;
	char string[BCTEXTLEN];
	int time_w = DEFAULT_TIMEW;

	day_text = 0;
	day_tumbler = 0;
	if(output_day)
	{
		gui->add_subwindow(day_text = new DayText(this, 
			x, 
			y, 
			50, 
			day_table, 
			TOTAL_DAYS, 
			day_table[*output_day]));
		x += day_text->get_w();
		time_w -= day_text->get_w();
		gui->add_subwindow(day_tumbler = new DayTumbler(this, 
			day_text, 
			x, 
			y));
		x += day_tumbler->get_w();
		time_w -= day_tumbler->get_w();
	}

	gui->add_subwindow(time_text = new TimeTextBox(this, 
		x, 
		y, 
		time_w, 
		Units::totext(string, 
			*output_time, 
			time_format)));
	time_text->set_separators(Units::format_to_separators(time_format));
}

int TimeEntry::get_h()
{
	return time_text->get_h();
}

int TimeEntry::get_w()
{
	int w = 0;
	if(day_text) w += day_text->get_w();
	if(day_tumbler) w += day_tumbler->get_w();
	w += time_text->get_w();
	return w;
}

void TimeEntry::reposition_window(int x, int y)
{
	int time_w = DEFAULT_TIMEW;

	this->x = x;
	this->y = y;
	
	if(output_day)
	{
		day_text->reposition_window(x, y);
		x += day_text->get_w();
		time_w -= day_text->get_w();
		day_tumbler->reposition_window(x, y);
		x += day_tumbler->get_w();
		time_w -= day_tumbler->get_w();
	}
	
	
	time_text->reposition_window(x, y, time_w);
}

void TimeEntry::update(int *day, double *time)
{
	char string[BCTEXTLEN];
	this->output_day = day;
	this->output_time = time;
	
	if(day)
	{
		day_text->update(day_table[*day]);
	}
	
	if(time)
		time_text->update(Units::totext(string, 	
			*time, 
			time_format));
}

int TimeEntry::handle_event()
{
	return 1;
}

DayText::DayText(TimeEntry *timeentry, 
		int x, 
		int y, 
		int w, 
		char **table, 
		int table_items,
		char *text)
 : BC_TextBox(x, y, w, 1, text)
{
	this->timeentry = timeentry;
	this->table = table;
	this->table_items = table_items;
}

int DayText::handle_event()
{
	*timeentry->output_day = TimeEntry::day_to_int(get_text());
	timeentry->handle_event();
	return 1;
}


DayTumbler::DayTumbler(TimeEntry *timeentry, 
		DayText *text, 
		int x, 
		int y)
 : BC_Tumbler(x, y)
{
	this->timeentry = timeentry;
	this->text = text;
}


int DayTumbler::handle_up_event()
{
	*timeentry->output_day += 1;
	if(*timeentry->output_day >= text->table_items)
		*timeentry->output_day = 0;
	text->update(text->table[*timeentry->output_day]);
	timeentry->handle_event();
	return 1;
}

int DayTumbler::handle_down_event()
{
	*timeentry->output_day -= 1;
	if(*timeentry->output_day < 0)
		*timeentry->output_day = text->table_items - 1;
	text->update(text->table[*timeentry->output_day]);
	timeentry->handle_event();
	return 1;
}



TimeTextBox::TimeTextBox(TimeEntry *timeentry, 
		int x, 
		int y,
		int w, 
		char *default_text)
 : BC_TextBox(x, 
 	y, 
	w, 
	1, 
	default_text)
{
	this->timeentry = timeentry;
}

int TimeTextBox::handle_event()
{
	*timeentry->output_time = Units::text_to_seconds(get_text(),
		48000,
		timeentry->time_format,
		0,
		0);
	timeentry->handle_event();
	return 1;
}
