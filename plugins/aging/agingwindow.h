#ifndef AGINGWINDOW_H
#define AGINGWINDOW_H

#include "guicast.h"

class AgingThread;
class AgingWindow;

#include "filexml.h"
#include "mutex.h"
#include "aging.h"

PLUGIN_THREAD_HEADER(AgingMain, AgingThread, AgingWindow)

class AgingColor;
class AgingScratches;
class AgingScratchCount;
class AgingPits;
class AgingPitCount;
class AgingDust;
class AgingDustCount;

class AgingWindow : public BC_Window
{
public:
	AgingWindow(AgingMain *client, int x, int y);
	~AgingWindow();

	int create_objects();
	int close_event();

	AgingMain *client;
	
	
	AgingColor *color;
	AgingScratches *scratches;
	AgingScratchCount *scratch_count;
	AgingPits *pits;
	AgingPitCount *pit_count;
	AgingDust *dust;
	AgingDustCount *dust_count;
};





class AgingColor : public BC_CheckBox
{
public:
	AgingColor(int x, int y, AgingMain *plugin);
	int handle_event();
	AgingMain *plugin;
};


class AgingScratches : public BC_CheckBox
{
public:
	AgingScratches(int x, int y, AgingMain *plugin);
	int handle_event();
	AgingMain *plugin;
};


class AgingScratchCount : public BC_ISlider
{
public:
	AgingScratchCount(int x, int y, AgingMain *plugin);
	int handle_event();
	AgingMain *plugin;
};

class AgingPits : public BC_CheckBox
{
public:
	AgingPits(int x, int y, AgingMain *plugin);
	int handle_event();
	AgingMain *plugin;
};

class AgingPitCount : public BC_ISlider
{
public:
	AgingPitCount(int x, int y, AgingMain *plugin);
	int handle_event();
	AgingMain *plugin;
};








class AgingDust : public BC_CheckBox
{
public:
	AgingDust(int x, int y, AgingMain *plugin);
	int handle_event();
	AgingMain *plugin;
};

class AgingDustCount : public BC_ISlider
{
public:
	AgingDustCount(int x, int y, AgingMain *plugin);
	int handle_event();
	AgingMain *plugin;
};







#endif
