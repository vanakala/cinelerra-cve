#ifndef EDITHANDLES_H
#define EDITHANDLES_H

#include "canvastools.h"
#include "edit.inc"
#include "guicast.h"
#include "mwindow.inc"
#include "trackcanvas.inc"

class EditHandle : public CanvasTool
{
public:
	EditHandle(MWindow *mwindow, 
		TrackCanvas *trackcanvas, 
		Edit *edit, 
		int side, 
		int x, 
		int y);
	virtual ~EditHandle();

	virtual int handle_event();
	int side;
};

class EditHandleIn : public EditHandle
{
public:
	EditHandleIn(MWindow *mwindow, 
		TrackCanvas *trackcanvas, 
		Edit *edit, 
		int x, 
		int y);
	virtual ~EditHandleIn();

	virtual int handle_event();
	int side;
};

class EditHandleOut : public EditHandle
{
public:
	EditHandleOut(MWindow *mwindow, 
		TrackCanvas *trackcanvas, 
		Edit *edit, 
		int x, 
		int y);
	virtual ~EditHandleOut();

	virtual int handle_event();
	int side;
};

class EditHandles : public CanvasTools
{
public:
	EditHandles(MWindow *mwindow, 
		TrackCanvas *trackcanvas);
	~EditHandles();
	
	void update();
};

#endif
