#ifndef HUEWINDOW_H
#define HUEWINDOW_H

#include "bcbase.h"

class HueThread;
class HueWindow;

#include "filexml.h"
#include "mutex.h"
#include "huesaturation.h"

class HueThread : public Thread
{
public:
	HueThread(HueMain *client);
	~HueThread();

	void run();

	Mutex gui_started; // prevent loading data until the GUI is started
	HueMain *client;
	HueWindow *window;
};

class HueSlider;
class SaturationSlider;
class ValueSlider;
class AutomatedFn;

class HueWindow : public BC_Window
{
public:
	HueWindow(HueMain *client);
	~HueWindow();
	
	int create_objects();
	int close_event();
	
	HueMain *client;
	HueSlider *hue_slider;
	SaturationSlider *saturation_slider;
	ValueSlider *value_slider;
	AutomatedFn *automation[3];
};

class HueSlider : public BC_ISlider
{
public:
	HueSlider(HueMain *client, int x, int y);
	~HueSlider();
	int handle_event();

	HueMain *client;
};

class SaturationSlider : public BC_ISlider
{
public:
	SaturationSlider(HueMain *client, int x, int y);
	~SaturationSlider();
	int handle_event();

	HueMain *client;
};

class ValueSlider : public BC_ISlider
{
public:
	ValueSlider(HueMain *client, int x, int y);
	~ValueSlider();
	int handle_event();

	HueMain *client;
};

class AutomatedFn : public BC_CheckBox
{
public:
	AutomatedFn(HueMain *client, HueWindow *window, int x, int y, int number);
	~AutomatedFn();
	int handle_event();

	HueMain *client;
	HueWindow *window;
	int number;
};


#endif
