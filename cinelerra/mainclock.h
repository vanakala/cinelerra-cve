#ifndef MAINCLOCK_H
#define MAINCLOCK_H

#include "guicast.h"
#include "mwindow.inc"
#include "mwindowgui.inc"

class MainClock : public BC_Title
{
public:
	MainClock(MWindow *mwindow, int x, int y, int w);
	~MainClock();
	
	void set_frame_offset(double value);
	void update(double position);
	
	MWindow *mwindow;
	double position_offset;
};


#endif
