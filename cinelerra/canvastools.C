#include "canvastools.h"
#include "mwindowgui.h"
#include "trackcanvas.h"



CanvasTool::CanvasTool(MWindow *mwindow, 
		TrackCanvas *trackcanvas,
		Edit *edit,
		int x,
		int y,
		VFrame **data)
 : BC_Button(x, y, data)
{
	this->mwindow = mwindow;
	this->trackcanvas = trackcanvas;
	this->edit = edit;
	visible = 1;
}

CanvasTool::~CanvasTool()
{
}




CanvasTools::CanvasTools(MWindow *mwindow,
		TrackCanvas *trackcanvas)
 : ArrayList<CanvasTool*>()
{
	this->mwindow = mwindow;
	this->trackcanvas = trackcanvas;
}

CanvasTools::~CanvasTools()
{
	remove_all_objects();
}

void CanvasTools::decrease_visible()
{
	for(int i = 0; i < total; i++)
		values[i]->visible--;
}

void CanvasTools::delete_invisible()
{
	for(int i = total - 1; i >= 0; i--)
		if(values[i]->visible < 1)
		{
			delete values[i];
			remove(values[i]);
		}	
}

// Region is visible on track canvas
int CanvasTools::visible(int x, int y, int w, int h)
{
	return MWindowGUI::visible(x, x + w, 0, trackcanvas->get_w()) &&
		MWindowGUI::visible(y, y + h, 0, trackcanvas->get_h());
}
