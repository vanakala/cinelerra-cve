#ifndef VPLAYBACKCURSOR_H
#define VPLAYBACKCURSOR_H

#include "mwindow.inc"
#include "tracking.h"
#include "playbackengine.inc"
#include "vwindow.inc"

class VTracking : public Tracking
{
public:
	VTracking(MWindow *mwindow, VWindow *vwindow);
	~VTracking();

	PlaybackEngine* get_playback_engine();
	void update_tracker(double position);
	void update_meters(int64_t position);
	void stop_meters();
	void draw();

	VWindow *vwindow;
};

#endif
