#ifndef UNSHARPWINDOW_H
#define UNSHARPWINDOW_H

#include "guicast.h"
#include "unsharp.inc"
#include "unsharpwindow.inc"

class UnsharpRadius : public BC_FPot
{
public:
	UnsharpRadius(UnsharpMain *plugin, int x, int y);
	int handle_event();
	UnsharpMain *plugin;
};

class UnsharpAmount : public BC_FPot
{
public:
	UnsharpAmount(UnsharpMain *plugin, int x, int y);
	int handle_event();
	UnsharpMain *plugin;
};

class UnsharpThreshold : public BC_IPot
{
public:
	UnsharpThreshold(UnsharpMain *plugin, int x, int y);
	int handle_event();
	UnsharpMain *plugin;
};

class UnsharpWindow : public BC_Window
{
public:
	UnsharpWindow(UnsharpMain *plugin, int x, int y);
	~UnsharpWindow();

	int create_objects();
	int close_event();
	void update();

	UnsharpRadius *radius;
	UnsharpAmount *amount;
	UnsharpThreshold *threshold;
	UnsharpMain *plugin;
};



PLUGIN_THREAD_HEADER(UnsharpMain, UnsharpThread, UnsharpWindow)




#endif
