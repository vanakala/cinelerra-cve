#ifndef HOLOWINDOW_H
#define HOLOWINDOW_H

#include "guicast.h"

class HoloThread;
class HoloWindow;

#include "filexml.h"
#include "mutex.h"
#include "holo.h"

class HoloThread : public Thread
{
public:
	HoloThread(HoloMain *client);
	~HoloThread();

	void run();

// prevent loading data until the GUI is started
 	Mutex gui_started, completion;
	HoloMain *client;
	HoloWindow *window;
};

class HoloWindow : public BC_Window
{
public:
	HoloWindow(HoloMain *client, int x, int y);
	~HoloWindow();

	int create_objects();
	int close_event();

	HoloMain *client;
};






#endif
