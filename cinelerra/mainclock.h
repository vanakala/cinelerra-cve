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
	
	void update(double position);
	
	MWindow *mwindow;
};


#endif
