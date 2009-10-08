
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

#ifndef TIMEENTRY_H
#define TIMEENTRY_H

class DayText;
class DayTumbler;
class TimeTextBox;
#define TOTAL_DAYS 8

#include "guicast.h"

// 3 part entry widget.
// part 1: day of the week
// part 2: day tumbler
// part 3: time of day
// Used by the Record GUI, Batch Rendering.

class TimeEntry
{
public:
	TimeEntry(BC_WindowBase *gui, 
		int x, 
		int y, 
		int *output_day, 
		double *output_time,
		int time_format);
	~TimeEntry();
	
	void create_objects();
	void time_to_hours(char *result, double time);
	void time_to_minutes(char *result, double time);
	void time_to_seconds(char *result, double time);
	virtual int handle_event();
	static int day_to_int(char *day);
	void update(int *day, double *time);
	void reposition_window(int x, int y);
	int get_h();
	int get_w();

	BC_WindowBase *gui;
	int x, y;
	DayText *day_text;
	DayTumbler *day_tumbler;
	TimeTextBox *time_text;
	double *output;
	static char *day_table[TOTAL_DAYS];
	int *output_day;
	double *output_time;
	int time_format;
};

class DayText : public BC_TextBox
{
public:
	DayText(TimeEntry *timeentry, 
		int x, 
		int y, 
		int w, 
		char **table, 
		int table_items,
		char *text);
	int handle_event();
	
	char **table;
	TimeEntry *timeentry;
	int table_items;
	int current_item;
};

class DayTumbler : public BC_Tumbler
{
public:
	DayTumbler(TimeEntry *timeentry, 
		DayText *text, 
		int x, 
		int y);

	int handle_up_event();
	int handle_down_event();
	
	TimeEntry *timeentry;
	DayText *text;
};

class TimeTextBox : public BC_TextBox
{
public:
	TimeTextBox(TimeEntry *timeentry, 
		int x, 
		int y,
		int w, 
		char *default_text);
	int handle_event();
	TimeEntry *timeentry;
};


#endif
