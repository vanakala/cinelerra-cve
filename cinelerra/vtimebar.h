#ifndef VTIMEBAR_H
#define VTIMEBAR_H


#include "timebar.h"
#include "vwindowgui.inc"

class VTimeBar : public TimeBar
{
public:
	VTimeBar(MWindow *mwindow, 
		VWindowGUI *gui,
		int x,
		int y,
		int w, 
		int h);

	int resize_event();
	EDL* get_edl();
	void draw_time();
	void update_preview();
	void select_label(double position);


	VWindowGUI *gui;
};


#endif
