#ifndef CANVASTOOLS_H
#define CANVASTOOLS_H

#include "edit.inc"
#include "guicast.h"
#include "mwindow.inc"
#include "trackcanvas.inc"
#include "vframe.inc"


// This was originally supposed to give a button feel to timeline elements.
// It is no longer used.



class CanvasTool : public BC_Button
{
public:
	CanvasTool(MWindow *mwindow, 
		TrackCanvas *trackcanvas,
		Edit *edit,
		int x,
		int y,
		VFrame **data);
	virtual ~CanvasTool();
	
	int visible;
	MWindow *mwindow;
	TrackCanvas *trackcanvas;
	Edit *edit;
	int x;
	int y;
};


class CanvasTools : public ArrayList<CanvasTool*>
{
public:
	CanvasTools(MWindow *mwindow,
		TrackCanvas *trackcanvas);
	virtual ~CanvasTools();
	
	void decrease_visible();
	void delete_invisible();
	int visible(int x, int y, int w, int h);

	MWindow *mwindow;
	TrackCanvas *trackcanvas;
};


#endif
