#ifndef DOTWINDOW_H
#define DOTWINDOW_H

#include "guicast.h"

class DotThread;
class DotWindow;

#include "filexml.h"
#include "mutex.h"
#include "dot.h"

PLUGIN_THREAD_HEADER(DotMain, DotThread, DotWindow)

class DotWindow : public BC_Window
{
public:
	DotWindow(DotMain *client, int x, int y);
	~DotWindow();

	int create_objects();
	int close_event();

	DotMain *client;
};






#endif
