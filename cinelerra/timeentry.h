#ifndef TIMEENTRY_H
#define TIMEENTRY_H

class DayText;
class DayTumbler;
class TimeTextBox;
#define TOTAL_DAYS 8

#include "guicast.h"

class TimeEntry
{
public:
	TimeEntry(BC_WindowBase *gui, 
		int x, 
		int y, 
		int *output_day, 
		double *output_time);
	~TimeEntry();
	
	void create_objects();
	void time_to_hours(char *result, double time);
	void time_to_minutes(char *result, double time);
	void time_to_seconds(char *result, double time);
	virtual int handle_event();
	static int day_to_int(char *day);
	void update(int *day, double *time);
	void reposition_window(int x, int y);

	BC_WindowBase *gui;
	int x, y;
	DayText *day_text;
	DayTumbler *day_tumbler;
	TimeTextBox *time_text;
	double *output;
	static char *day_table[TOTAL_DAYS];
	int *output_day;
	double *output_time;
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
