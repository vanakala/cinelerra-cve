#ifndef R0TATEWINDOW_H
#define R0TATEWINDOW_H

#include "bcbase.h"

class RotateThread;
class RotateWindow;

#include "filexml.h"
#include "mutex.h"
#include "rotate.h"

class RotateThread : public Thread
{
public:
	RotateThread(RotateMain *client);
	~RotateThread();

	void run();

	Mutex gui_started; // prevent loading data until the GUI is started
	RotateMain *client;
	RotateWindow *window;
};

class RotateToggle;
class RotateFine;

class RotateWindow : public BC_Window
{
public:
	RotateWindow(RotateMain *client);
	~RotateWindow();

	int create_objects();
	int close_event();
    int update_parameters();
	int update_fine();
	int update_toggles();

	RotateMain *client;
	RotateToggle *toggle0;
	RotateToggle *toggle90;
	RotateToggle *toggle180;
	RotateToggle *toggle270;
	RotateFine *fine;
};

class RotateToggle : public BC_Radial
{
public:
	RotateToggle(RotateWindow *window, RotateMain *client, int init_value, int x, int y, int value, char *string);
	~RotateToggle();
	int handle_event();

	RotateMain *client;
    RotateWindow *window;
    int value;
};

class RotateFine : public BC_IPot
{
public:
	RotateFine(RotateWindow *window, RotateMain *client, int x, int y);
	~RotateFine();
	int handle_event();

	RotateMain *client;
    RotateWindow *window;
};


#endif
