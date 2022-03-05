// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef MAINPROGRESS_H
#define MAINPROGRESS_H

#include "arraylist.h"
#include "bcprogressbox.inc"
#include "bcprogress.inc"
#include "bcwindowbase.inc"
#include "datatype.h"
#include "mainprogress.inc"
#include "mwindowgui.inc"
#include "bctimer.inc"

// Generic progress bar for the MainProgress
class MainProgress;

class MainProgressBar
{
public:
	MainProgressBar(MainProgress *mainprogress);
	~MainProgressBar();

	friend class MainProgress;

	void stop_progress();
	int is_cancelled();

	void update_title(const char *fmt, ...)
		__attribute__ ((__format__(__printf__, 2, 3)));
	void update_current_title(const char *fmt, ...)
		__attribute__ ((__format__(__printf__, 2, 3)));
	void update_length(int64_t length);
	int update(int64_t value);
	int update(ptstime value);
	void get_time(char *text);
	double get_time(); 

// Only defined if this is a separate window;
	BC_ProgressBox *progress_box;
// Only defined if this is the main progress bar
	BC_ProgressBar *progress_bar;
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
	MainProgress(MWindowGUI *gui);

// Start a progress sequence and return the bar.
// use_window - force opening of a new window if 1.
	MainProgressBar* start_progress(const char *text,
		int64_t total_length, 
		int use_window = 0);
	MainProgressBar* start_progress(const char *text,
		ptstime total_len_pts, 
		int use_window = 0);
	void end_progress(MainProgressBar* progress_bar);

	ArrayList<MainProgressBar*> progress_bars;
	MainProgressBar *mwindow_progress;

	MWindowGUI *gui;
// For mwindow progress
	int cancelled;
};


#endif
