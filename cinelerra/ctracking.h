#ifndef CPLAYBACKCURSOR_H
#define CPLAYBACKCURSOR_H

#include "cwindow.inc"
#include "mwindow.inc"
#include "tracking.h"
#include "playbackengine.inc"

class CTracking : public Tracking
{
public:
	CTracking(MWindow *mwindow, CWindow *cwindow);
	~CTracking();

	PlaybackEngine* get_playback_engine();
	void update_tracker(double position);
// Move Track Canvas left or right to track playback
	int update_scroll(double position);

// Move sliders and insertion point to track playback
	int start_playback(double new_position);
	int stop_playback();
	void draw();

	CWindow *cwindow;
};

#endif
