#ifndef FLIPWINDOW_H
#define FLIPWINDOW_H


class FlipThread;
class FlipWindow;

#include "filexml.inc"
#include "flip.h"
#include "mutex.h"
#include "pluginvclient.h"

PLUGIN_THREAD_HEADER(FlipMain, FlipThread, FlipWindow)

class FlipToggle;

class FlipWindow : public BC_Window
{
public:
	FlipWindow(FlipMain *client, int x, int y);
	~FlipWindow();
	
	int create_objects();
	int close_event();
	
	FlipMain *client;
	FlipToggle *flip_vertical;
	FlipToggle *flip_horizontal;
};

class FlipToggle : public BC_CheckBox
{
public:
	FlipToggle(FlipMain *client, int *output, char *string, int x, int y);
	~FlipToggle();
	int handle_event();

	FlipMain *client;
	int *output;
};


#endif
