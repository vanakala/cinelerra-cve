#ifndef QUIT_H
#define QUIT_H

#include "guicast.h"
#include "mwindow.inc"
#include "savefile.inc"

class Quit : public BC_MenuItem, public Thread
{
public:
	Quit(MWindow *mwindow);
	int create_objects(Save *save);
	int handle_event();
	void run();
	
	Save *save;
	MWindow *mwindow;
};

#endif
