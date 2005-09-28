#ifndef TRACKSCROLL_H
#define TRACKSCROLL_H


#include "guicast.h"
#include "mwindow.inc"
#include "mwindowgui.inc"

class TrackScroll : public BC_ScrollBar
{
public:
	TrackScroll(MWindow *mwindow, MWindowGUI *gui, int x, int y, int h);
	~TrackScroll();

	int create_objects(int top, int bottom);
	int resize_event();
	int flip_vertical(int top, int bottom);
	int update();               // reflect new track view
	long get_distance();
	int handle_event();

	MWindowGUI *gui;
	MWindow *mwindow;
	long old_position;
};

#endif
