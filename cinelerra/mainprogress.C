
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

#include "bcprogressbox.h"
#include "language.h"
#include "mainprogress.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "statusbar.h"
#include "bctimer.h"

#include <string.h>

MainProgressBar::MainProgressBar(MWindow *mwindow, MainProgress *mainprogress)
{
	progress_box = 0;
	progress_bar = 0;
	this->mwindow = mwindow;
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
		mwindow->gui->statusbar->default_message();
	}
}

int MainProgressBar::is_cancelled()
{
	if(progress_box)
	{
//printf("MainProgressBar::is_cancelled 1 %d\n", progress_box->is_cancelled());
		return progress_box->is_cancelled();
	}
	else
	if(progress_bar)
	{
		return mainprogress->cancelled;
	}

	return 0;
}

void MainProgressBar::update_title(char *string, int default_)
{
	if(default_) strcpy(default_title, string);
//printf("MainProgressBar::update_title %p %p %s\n", progress_bar, progress_box, string);

	if(progress_box)
	{
		progress_box->update_title(string, 1);
	}
	else
	if(progress_bar)
	{
		mwindow->gui->lock_window("MainProgressBar::update_title");
		mwindow->gui->show_message(string);
		mwindow->gui->unlock_window();
	}
}

void MainProgressBar::update_length(int64_t length)
{
	this->length = length;
//printf("MainProgressBar::update_length %d\n", length);
	if(progress_box)
	{
		progress_box->update_length(length, 1);
	}
	else
	if(progress_bar)
	{
		mwindow->gui->lock_window("MainProgressBar::update_length");
		progress_bar->update_length(length);
		mwindow->gui->unlock_window();
	}
}

int MainProgressBar::update(int64_t value)
{
// Print ETA on title
	double current_eta = (double)eta_timer->get_scaled_difference(1000);

//printf("MainProgressBar::update %f %f %f\n", current_eta, last_eta, current_eta - last_eta);
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

//printf("MainProgressBar::update %f %d %d %f\n", current_eta, length, value, eta);
 		Units::totext(time_string, 
 			eta,
 			TIME_HMS2);
// 		sprintf(time_string, 
// 			"%dh%dm%ds", 
// 			(int64_t)eta / 3600,
// 			((int64_t)eta / 60) % 60,
// 			(int64_t)eta % 60);

		sprintf(string, _("%s ETA: %s"), 
			default_title, 
			time_string);
		update_title(string, 0);

		last_eta = (int64_t)current_eta;
	}

	if(progress_box)
	{
		progress_box->update(value, 1);
	}
	else
	if(progress_bar)
	{
		mwindow->gui->lock_window("MainProgressBar::update");
		progress_bar->update(value);
		mwindow->gui->unlock_window();
	}

	return is_cancelled();
}

void MainProgressBar::get_time(char *text)
{
	double current_time = (double)eta_timer->get_scaled_difference(1);
//printf("MainProgressBar::get_time %f\n", current_time);
	Units::totext(text, 
			current_time,
			TIME_HMS2);
}

double MainProgressBar::get_time()
{
	return (double)eta_timer->get_scaled_difference(1);
}











MainProgress::MainProgress(MWindow *mwindow, MWindowGUI *gui)
{
	this->mwindow = mwindow;
	this->gui = gui;
	mwindow_progress = 0;
	cancelled = 0;
}


MainProgress::~MainProgress()
{
}

MainProgressBar* MainProgress::start_progress(char *text, 
	int64_t total_length,
	int use_window)
{
	MainProgressBar *result = 0;

// Default to main window
	if(!mwindow_progress && !use_window)
	{
		mwindow_progress = new MainProgressBar(mwindow, this);
		mwindow_progress->progress_bar = gui->statusbar->main_progress;
		mwindow_progress->progress_bar->update_length(total_length);
		mwindow_progress->update_title(text);
		result = mwindow_progress;
		cancelled = 0;
	}

	if(!result)
	{
		result = new MainProgressBar(mwindow, this);
		progress_bars.append(result);
		result->progress_box = new BC_ProgressBox(gui->get_abs_cursor_x(1), 
			gui->get_abs_cursor_y(1), 
			text, 
			total_length);
	}

	result->length = total_length;
	strcpy(result->default_title, text);
	result->start();
	return result;
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
