#ifndef RGB601WINDOW_H
#define RGB601WINDOW_H

#include "guicast.h"

class RGB601Thread;
class RGB601Window;

#include "filexml.h"
#include "mutex.h"
#include "rgb601.h"

PLUGIN_THREAD_HEADER(RGB601Main, RGB601Thread, RGB601Window)

class RGB601Direction : public BC_CheckBox
{
public:
	RGB601Direction(RGB601Window *window, int x, int y, int *output, int true_value, char *text);
	~RGB601Direction();
	
	int handle_event();
	RGB601Window *window;
	int *output;
	int true_value;
};

class RGB601Window : public BC_Window
{
public:
	RGB601Window(RGB601Main *client, int x, int y);
	~RGB601Window();
	
	int create_objects();
	int close_event();
	void update();

	RGB601Main *client;
	BC_CheckBox *forward, *reverse;
};

#endif
