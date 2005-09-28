#ifndef GWINDOW_H
#define GWINDOW_H


#include "gwindowgui.inc"
#include "mwindow.inc"
#include "thread.h"

class GWindow : public Thread
{
public:
	GWindow(MWindow *mwindow);
	void run();
	void create_objects();
	MWindow *mwindow;
	GWindowGUI *gui;
};



#endif
