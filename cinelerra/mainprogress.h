#ifndef MAINPROGRESS_H
#define MAINPROGRESS_H

#include "arraylist.h"
#include "bcprogressbox.inc"
#include "guicast.h"
#include "mainprogress.inc"
#include "mwindow.inc"
#include "mwindowgui.inc"
#include "timer.inc"

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
	void update_length(long length);
	int update(long value);
	void get_time(char *text);

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
	long last_eta;
	long length;

private:
	void start();
};

// Controls all progressbars and locations

class MainProgress
{
public:
	MainProgress(MWindow *mwindow, MWindowGUI *gui);
	~MainProgress();

// Start a progress sequence and return the bar
	MainProgressBar* start_progress(char *text, long total_length);
	void end_progress(MainProgressBar* progress_bar);
	void get_time(char *text);

	ArrayList<MainProgressBar*> progress_bars;
	MainProgressBar *mwindow_progress;

	MWindow *mwindow;
	MWindowGUI *gui;
// For mwindow progress
	int cancelled;
};


#endif
