#ifndef MAINXSCROLL_H
#define MAINXSCROLL_H

#include "guicast.h"
#include "mwindow.inc"
#include "mwindowgui.inc"

class SampleScroll : public BC_ScrollBar
{
public:
	SampleScroll(MWindow *mwindow, MWindowGUI *gui, int pixels);
	~SampleScroll();

	int create_objects();
	int flip_vertical();
	int in_use();
	int resize_event();
	int set_position();
	int handle_event();
	double oldposition;

private:
	MWindowGUI *gui;
	MWindow *mwindow;
};



#endif
