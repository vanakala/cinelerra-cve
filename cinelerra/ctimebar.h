#ifndef CTIMEBAR_H
#define CTIMEBAR_H




#include "cwindowgui.inc"
#include "timebar.h"





class CTimeBar : public TimeBar
{
public:
	CTimeBar(MWindow *mwindow, 
		CWindowGUI *gui,
		int x,
		int y,
		int w, 
		int h);

	int resize_event();
	EDL* get_edl();
	void draw_time();
	void update_preview();
	void select_label(double position);

	CWindowGUI *gui;
};


#endif
