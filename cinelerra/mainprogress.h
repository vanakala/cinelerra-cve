
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

#ifndef MAINPROGRESS_H
#define MAINPROGRESS_H

#include "arraylist.h"
#include "bcprogressbox.inc"
#include "guicast.h"
#include "mainprogress.inc"
#include "mwindow.inc"
#include "mwindowgui.inc"
#include "bctimer.inc"

// Generic progress bar for the MainProgress
class MainProgress;

class MainProgressBar
{
public:
	MainProgressBar(MWindow *mwindow, MainProgress *mainprogress);
	~MainProgressBar();

	friend class MainProgress;

	void stop_progress();
	int is_cancelled();
	void update_title(char *string, int default_ = 1);
	void update_length(int64_t length);
	int update(int64_t value);
	void get_time(char *text);
	double get_time(); 

// Only defined if this is a separate window;
	BC_ProgressBox *progress_box;
// Only defined if this is the main progress bar
	BC_ProgressBar *progress_bar;
	MWindow *mwindow;
	MainProgress *mainprogress;
// Title assigned by user
	char default_title[BCTEXTLEN];
	Timer *eta_timer;
// Last time eta was updated
	int64_t last_eta;
	int64_t length;

private:
	void start();
};

// Controls all progressbars and locations

class MainProgress
{
public:
	MainProgress(MWindow *mwindow, MWindowGUI *gui);
	~MainProgress();

// Start a progress sequence and return the bar.
// use_window - force opening of a new window if 1.
	MainProgressBar* start_progress(char *text, 
		int64_t total_length, 
		int use_window = 0);
	void end_progress(MainProgressBar* progress_bar);

	ArrayList<MainProgressBar*> progress_bars;
	MainProgressBar *mwindow_progress;

	MWindow *mwindow;
	MWindowGUI *gui;
// For mwindow progress
	int cancelled;
};


#endif
