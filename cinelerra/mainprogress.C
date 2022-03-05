// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bcprogress.h"
#include "bcprogressbox.h"
#include "language.h"
#include "mainprogress.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "statusbar.h"
#include "bctimer.h"
#include "units.h"

#include <string.h>

#define MAINPROGRESS_COEF 1e6

MainProgressBar::MainProgressBar(MainProgress *mainprogress)
{
	progress_box = 0;
	progress_bar = 0;
	this->mainprogress = mainprogress;
	eta_timer = new Timer;
}

MainProgressBar::~MainProgressBar()
{
	if(progress_box) delete progress_box;

	if(mainprogress->mwindow_progress == this)
	{
		mainprogress->mwindow_progress = 0;
	}
	else
	{
		mainprogress->progress_bars.remove(this);
	}
	delete eta_timer;
}

void MainProgressBar::start()
{
	if(progress_box)
	{
		progress_box->start();
	}
	eta_timer->update();
	last_eta = 0;
}

void MainProgressBar::stop_progress()
{
	if(progress_box)
	{
		progress_box->stop_progress();
	}
	else
	if(progress_bar)
	{
		progress_bar->update(0);
		mwindow_global->default_message();
	}
}

int MainProgressBar::is_cancelled()
{
	if(progress_box)
	{
		return progress_box->is_cancelled();
	}
	else
	if(progress_bar)
	{
		return mainprogress->cancelled;
	}

	return 0;
}

void MainProgressBar::update_current_title(const char *fmt, ...)
{
	char bufr[BCTEXTLEN];
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(bufr, BCTEXTLEN, fmt, ap);
	va_end(ap);

	if(progress_box)
	{
		progress_box->update_title(bufr, 1);
	}
	else
	if(progress_bar)
		mwindow_global->show_message(bufr);
}

void MainProgressBar::update_title(const char *fmt, ...)
{
	char bufr[BCTEXTLEN];
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(bufr, BCTEXTLEN, fmt, ap);
	va_end(ap);

	strcpy(default_title, bufr);

	if(progress_box)
	{
		progress_box->update_title(bufr, 1);
	}
	else
	if(progress_bar)
		mwindow_global->show_message(bufr);
}

void MainProgressBar::update_length(int64_t length)
{
	this->length = length;
	if(progress_box)
	{
		progress_box->update_length(length, 1);
	}
	else
	if(progress_bar)
		progress_bar->update_length(length);
}

int MainProgressBar::update(int64_t value)
{
// Print ETA on title
	double current_eta = (double)eta_timer->get_scaled_difference(1000);

	if(current_eta - last_eta > 1000)
	{
		char string[BCTEXTLEN];
		char time_string[BCTEXTLEN];
		double eta;

		if(value > 0.001)
			eta = (double)current_eta / 
				1000 * 
				length / 
				value - 
				current_eta / 
				1000;
		else
			eta = 0;

		Units::totext(time_string, 
			eta,
			TIME_HMS2);

		update_current_title(_("%s ETA: %s"), 
			default_title, 
			time_string);

		last_eta = (int64_t)current_eta;
	}

	if(progress_box)
	{
		progress_box->update(value, 1);
	}
	else
	if(progress_bar)
		progress_bar->update(value);

	return is_cancelled();
}

int MainProgressBar::update(ptstime value)
{
	return update((int64_t)(value * MAINPROGRESS_COEF));
}

void MainProgressBar::get_time(char *text)
{
	double current_time = (double)eta_timer->get_scaled_difference(1);
	Units::totext(text, 
			current_time,
			TIME_HMS2);
}

double MainProgressBar::get_time()
{
	return (double)eta_timer->get_scaled_difference(1);
}


MainProgress::MainProgress(MWindowGUI *gui)
{
	this->gui = gui;
	mwindow_progress = 0;
	cancelled = 0;
}


MainProgressBar* MainProgress::start_progress(const char *text,
	int64_t total_length,
	int use_window)
{
	int cx, cy;

	MainProgressBar *result = 0;

// Default to main window
	if(!mwindow_progress && !use_window)
	{
		mwindow_progress = new MainProgressBar(this);
		mwindow_progress->progress_bar = gui->statusbar->main_progress;
		mwindow_progress->progress_bar->update_length(total_length);
		mwindow_progress->update_title("%s", text);
		result = mwindow_progress;
		cancelled = 0;
	}

	if(!result)
	{
		result = new MainProgressBar(this);
		progress_bars.append(result);
		mwindow_global->get_abs_cursor_pos(&cx, &cy);
		result->progress_box = new BC_ProgressBox(cx, cy,
			text, total_length);
	}

	result->length = total_length;
	strcpy(result->default_title, text);
	result->start();
	return result;
}

MainProgressBar* MainProgress::start_progress(const char *text,
	ptstime total_length_pts,
	int use_window)
{
	int64_t total_length = (int64_t)(total_length_pts * MAINPROGRESS_COEF);
	return start_progress(text, total_length, use_window);
}


void MainProgress::end_progress(MainProgressBar* progress_bar)
{
	if(mwindow_progress == progress_bar)
	{
		delete mwindow_progress;
		mwindow_progress = 0;
	}
	else
	{
		delete progress_bar;
		progress_bars.remove(progress_bar);
	}
}
