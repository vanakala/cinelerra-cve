#ifndef HOLOWINDOW_H
#define HOLOWINDOW_H

#include "guicast.h"

class HoloThread;
class HoloWindow;

#include "filexml.h"
#include "mutex.h"
#include "holo.h"


PLUGIN_THREAD_HEADER(HoloMain, HoloThread, HoloWindow)

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
