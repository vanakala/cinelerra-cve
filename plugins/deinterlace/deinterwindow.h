#ifndef DEINTERWINDOW_H
#define DEINTERWINDOW_H


class DeInterlaceThread;
class DeInterlaceWindow;

#include "guicast.h"
#include "mutex.h"
#include "deinterlace.h"
#include "pluginclient.h"

PLUGIN_THREAD_HEADER(DeInterlaceMain, DeInterlaceThread, DeInterlaceWindow);

class DeInterlaceOption;
class DeInterlaceAdaptive;
class DeInterlaceThreshold;

class DeInterlaceWindow : public BC_Window
{
public:
	DeInterlaceWindow(DeInterlaceMain *client, int x, int y);
	~DeInterlaceWindow();
	
	int create_objects();
	int close_event();
	int set_mode(int mode, int recursive);
	void get_status_string(char *string, int changed_rows);
	
	DeInterlaceMain *client;
	DeInterlaceOption *odd_fields;
	DeInterlaceOption *even_fields;
	DeInterlaceOption *average_fields;
	DeInterlaceOption *swap_fields;
	DeInterlaceOption *avg_even;
	DeInterlaceOption *avg_odd;
	DeInterlaceOption *none;
	DeInterlaceAdaptive *adaptive;
	DeInterlaceThreshold *threshold;
	BC_Title *status;
};

class DeInterlaceOption : public BC_Radial
{
public:
	DeInterlaceOption(DeInterlaceMain *client, 
		DeInterlaceWindow *window, 
		int output, 
		int x, 
		int y, 
		char *text);
	~DeInterlaceOption();
	int handle_event();

	DeInterlaceMain *client;
	DeInterlaceWindow *window;
	int output;
};

class DeInterlaceAdaptive : public BC_CheckBox
{
public:
	DeInterlaceAdaptive(DeInterlaceMain *client, int x, int y);
	int handle_event();
	DeInterlaceMain *client;
};

class DeInterlaceThreshold : public BC_IPot
{
public:
	DeInterlaceThreshold(DeInterlaceMain *client, int x, int y);
	int handle_event();
	DeInterlaceMain *client;
};




#endif
