#ifndef MTIMEBAR_H
#define MTIMEBAR_H


#include "mwindowgui.inc"
#include "timebar.h"


class MTimeBar : public TimeBar
{
public:
	MTimeBar(MWindow *mwindow, 
		MWindowGUI *gui,
		int x, 
		int y,
		int w,
		int h);
	
	void draw_time();
	void draw_range();
	void stop_playback();
	int resize_event();
	int test_preview(int buttonpress);
	long position_to_pixel(double position);
	void select_label(double position);

	MWindowGUI *gui;
};



#endif
