#ifndef TIMEAVGWINDOW_H
#define TIMEAVGWINDOW_H


class TimeAvgThread;
class TimeAvgWindow;
class TimeAvgAccum;
class TimeAvgAvg;
class TimeAvgOr;
class TimeAvgParanoid;
class TimeAvgNoSubtract;

#include "guicast.h"
#include "mutex.h"
#include "timeavg.h"

PLUGIN_THREAD_HEADER(TimeAvgMain, TimeAvgThread, TimeAvgWindow)

class TimeAvgSlider;

class TimeAvgWindow : public BC_Window
{
public:
	TimeAvgWindow(TimeAvgMain *client, int x, int y);
	~TimeAvgWindow();
	
	int create_objects();
	int close_event();
	
	TimeAvgMain *client;
	TimeAvgSlider *total_frames;
	TimeAvgAccum *accum;
	TimeAvgAvg *avg;
	TimeAvgOr *inclusive_or;
	TimeAvgParanoid *paranoid;
	TimeAvgNoSubtract *no_subtract;
};

class TimeAvgSlider : public BC_ISlider
{
public:
	TimeAvgSlider(TimeAvgMain *client, int x, int y);
	~TimeAvgSlider();
	int handle_event();

	TimeAvgMain *client;
};

class TimeAvgAccum : public BC_Radial
{
public:
	TimeAvgAccum(TimeAvgMain *client, TimeAvgWindow *gui, int x, int y);
	int handle_event();
	TimeAvgMain *client;
	TimeAvgWindow *gui;
};

class TimeAvgAvg : public BC_Radial
{
public:
	TimeAvgAvg(TimeAvgMain *client, TimeAvgWindow *gui, int x, int y);
	int handle_event();
	TimeAvgMain *client;
	TimeAvgWindow *gui;
};

class TimeAvgOr : public BC_Radial
{
public:
	TimeAvgOr(TimeAvgMain *client, TimeAvgWindow *gui, int x, int y);
	int handle_event();
	TimeAvgMain *client;
	TimeAvgWindow *gui;
};

class TimeAvgParanoid : public BC_CheckBox
{
public:
	TimeAvgParanoid(TimeAvgMain *client, int x, int y);
	int handle_event();
	TimeAvgMain *client;
};

class TimeAvgNoSubtract : public BC_CheckBox
{
public:
	TimeAvgNoSubtract(TimeAvgMain *client, int x, int y);
	int handle_event();
	TimeAvgMain *client;
};

#endif
