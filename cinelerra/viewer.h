#ifndef VIEWER_H
#define VIEWER_H

#include "mwindow.inc"
#include "thread.h"

class VWindow : public Thread
{
public:
	VWindow(MWindow *mwindow);
	~VWindow();
	
	MWindow *mwindow;
};

#endif
