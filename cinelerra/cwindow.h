#ifndef CWINDOW_H
#define CWINDOW_H

#include "auto.inc"
#include "autos.inc"
#include "cplayback.inc"
#include "ctracking.inc"
#include "cwindowgui.inc"
#include "mwindow.inc"
#include "thread.h"
#include "track.inc"

class CWindow : public Thread
{
public:
	CWindow(MWindow *mwindow);
	~CWindow();
	
    int create_objects();
// Position is inclusive of the other 2
	void update(int position, 
		int overlays, 
		int tool_window, 
		int operation = 0,
		int timebar = 0);
	void run();
// Get keyframe for editing in the CWindow
	Track* calculate_affected_track();
	Auto* calculate_affected_auto(Autos *autos, int create = 1);

	int destination;
	MWindow *mwindow;
    CWindowGUI *gui;
	
	CTracking *playback_cursor;
	CPlayback *playback_engine;
};

#endif
