#ifndef TRACKING_H
#define TRACKING_H

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "condition.inc"
#include "mwindow.inc"
#include "mwindowgui.inc"
#include "playbackengine.inc"
#include "thread.h"
#include "bctimer.h"

class Tracking : public Thread
{
public:
	Tracking(MWindow *mwindow);
	virtual ~Tracking();

	void create_objects();
	virtual int start_playback(double new_position);
	virtual int stop_playback();

// Called by the tracker to get the current position
	virtual PlaybackEngine* get_playback_engine();
	virtual double get_tracking_position();
// Update position displayed
	virtual void update_tracker(double position);
// Update meters
	virtual void update_meters(int64_t position);
	virtual void stop_meters();
	int get_pixel(double position);

// Erase cursor if it's visible.  Called by start_playback
// Draw new cursor at last_position if invisible
	virtual void draw();



	void run();

// Values to return from playback_engine to update_meter .
// Use ArrayList to simplify module counting
	ArrayList<double> module_levels;
	int state;










	void show_playback_cursor(int64_t position);
	int view_follows_playback;
// Delay until startup
	Condition *startup_lock;
	MWindow *mwindow;
	MWindowGUI *gui;
	double last_position;
	int follow_loop;
	int64_t current_offset;
	int reverse;
	int double_speed;
	Timer timer;
// Pixel of last drawn cursor
	int pixel;
// Cursor is visible
	int visible;
};

#endif
