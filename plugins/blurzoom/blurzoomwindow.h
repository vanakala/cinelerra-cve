#ifndef BLURZOOMWINDOW_H
#define BLURZOOMWINDOW_H

#include "guicast.h"

class BlurZoomThread;
class BlurZoomWindow;

#include "filexml.h"
#include "mutex.h"
#include "blurzoom.h"

class BlurZoomThread : public Thread
{
public:
	BlurZoomThread(BlurZoomMain *client);
	~BlurZoomThread();

	void run();

// prevent loading data until the GUI is started
 	Mutex gui_started, completion;
	BlurZoomMain *client;
	BlurZoomWindow *window;
};

class BlurZoomWindow : public BC_Window
{
public:
	BlurZoomWindow(BlurZoomMain *client, int x, int y);
	~BlurZoomWindow();

	int create_objects();
	int close_event();

	BlurZoomMain *client;
};








#endif
