#ifndef WHIRLWINDOW_H
#define WHIRLWINDOW_H

#include "guicast.h"

class WhirlThread;
class WhirlWindow;

#include "filexml.h"
#include "mutex.h"
#include "whirl.h"

class WhirlThread : public Thread
{
public:
	WhirlThread(WhirlMain *client);
	~WhirlThread();

	void run();

	Mutex gui_started; // prevent loading data until the GUI is started
	WhirlMain *client;
	WhirlWindow *window;
};

class AngleSlider;
class PinchSlider;
class RadiusSlider;
class AutomatedFn;

class WhirlWindow : public BC_Window
{
public:
	WhirlWindow(WhirlMain *client);
	~WhirlWindow();
	
	int create_objects();
	int close_event();
	
	WhirlMain *client;
	AngleSlider *angle_slider;
	PinchSlider *pinch_slider;
	RadiusSlider *radius_slider;
	AutomatedFn *automation[3];
};

class AngleSlider : public BC_ISlider
{
public:
	AngleSlider(WhirlMain *client, int x, int y);
	~AngleSlider();
	int handle_event();

	WhirlMain *client;
};

class PinchSlider : public BC_ISlider
{
public:
	PinchSlider(WhirlMain *client, int x, int y);
	~PinchSlider();
	int handle_event();

	WhirlMain *client;
};

class RadiusSlider : public BC_ISlider
{
public:
	RadiusSlider(WhirlMain *client, int x, int y);
	~RadiusSlider();
	int handle_event();

	WhirlMain *client;
};

class AutomatedFn : public BC_CheckBox
{
public:
	AutomatedFn(WhirlMain *client, WhirlWindow *window, int x, int y, int number);
	~AutomatedFn();
	int handle_event();

	WhirlMain *client;
	WhirlWindow *window;
	int number;
};


#endif
