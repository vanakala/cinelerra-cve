#ifndef FLIPWINDOW_H
#define FLIPWINDOW_H

#include "bcbase.h"

class InvertThread;
class InvertWindow;

#include "filexml.h"
#include "mutex.h"
#include "invert.h"

class InvertThread : public Thread
{
public:
	InvertThread(InvertMain *client);
	~InvertThread();

	void run();

	Mutex gui_started; // prevent loading data until the GUI is started
	InvertMain *client;
	InvertWindow *window;
};

class InvertToggle;

class InvertWindow : public BC_Window
{
public:
	InvertWindow(InvertMain *client);
	~InvertWindow();
	
	int create_objects();
	int close_event();
	
	InvertMain *client;
	InvertToggle *invert;
};

class InvertToggle : public BC_CheckBox
{
public:
	InvertToggle(InvertMain *client, int *output, int x, int y);
	~InvertToggle();
	int handle_event();

	InvertMain *client;
	int *output;
};


#endif
