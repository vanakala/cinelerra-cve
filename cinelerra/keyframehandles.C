#include "keyframehandles.h"
#include "mwindow.h"
#include "theme.h"


KeyframeHandle::KeyframeHandle(MWindow *mwindow, 
		TrackCanvas *trackcanvas,
		Track *track,
		long position, 
		int x,
		int y)
 : CanvasTool(mwindow,
 	trackcanvas,
	0,
	x, 
	y, 
	0)
{
	this->track = track;
	this->position = position;
}

KeyframeHandle::~KeyframeHandle()
{
}

int KeyframeHandle::handle_event()
{
 	return 1;
}


KeyframeHandles::KeyframeHandles(MWindow *mwindow, 
	TrackCanvas *trackcanvas)
 : CanvasTools(mwindow, trackcanvas)
{
}

KeyframeHandles::~KeyframeHandles()
{
}
