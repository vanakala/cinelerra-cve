#ifndef TRANSITIONHANDLES_H
#define TRANSITIONHANDLES_H

#include "canvastools.h"
#include "edit.inc"
#include "guicast.h"
#include "mwindow.inc"
#include "trackcanvas.inc"

class TransitionHandle : public CanvasTool
{
public:
	TransitionHandle(MWindow *mwindow, 
		TrackCanvas *trackcanvas,
		Edit *edit,
		int x,
		int y);
	~TransitionHandle();
	
	int handle_event();
};


class TransitionHandles : public CanvasTools
{
public:
	TransitionHandles(MWindow *mwindow, 
		TrackCanvas *trackcanvas);
	~TransitionHandles();
	
	void update();
};





#endif
