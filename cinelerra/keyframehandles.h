#ifndef KEYFRAMEHANDLES_H
#define KEYFRAMEHANDLES_H

#include "canvastools.h"
#include "guicast.h"
#include "mwindow.inc"
#include "track.inc"
#include "trackcanvas.inc"

class KeyframeHandle : public CanvasTool
{
public:
	KeyframeHandle(MWindow *mwindow, 
		TrackCanvas *trackcanvas,
		Track *track,
		int64_t position,
		int x,
		int y);
	~KeyframeHandle();
	
	int handle_event();
	
	Track *track;
	int64_t position;
};


class KeyframeHandles : public CanvasTools
{
public:
	KeyframeHandles(MWindow *mwindow, 
		TrackCanvas *trackcanvas);
	~KeyframeHandles();
};


#endif
