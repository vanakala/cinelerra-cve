#ifndef BURNWINDOW_H
#define BURNWINDOW_H

#include "guicast.h"

class BurnThread;
class BurnWindow;

#include "filexml.h"
#include "mutex.h"
#include "burn.h"


PLUGIN_THREAD_HEADER(BurnMain, BurnThread, BurnWindow)



class BurnWindow : public BC_Window
{
public:
	BurnWindow(BurnMain *client, int x, int y);
	~BurnWindow();

	int create_objects();
	int close_event();

	BurnMain *client;
};








#endif
