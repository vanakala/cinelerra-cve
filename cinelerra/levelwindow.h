#ifndef LEVELWINDOW_H
#define LEVELWINDOW_H

#include "levelwindowgui.inc"
#include "mwindow.inc"
#include "thread.h"


class LevelWindow : public Thread
{
public:
	LevelWindow(MWindow *mwindow);
	~LevelWindow();
	
	int create_objects();
	void run();
	
	LevelWindowGUI *gui;
	MWindow *mwindow;
};





#endif
