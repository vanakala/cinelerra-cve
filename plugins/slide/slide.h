#ifndef SLIDE_H
#define SLIDE_H

class SlideMain;
class SlideWindow;


#include "overlayframe.inc"
#include "pluginvclient.h"
#include "vframe.inc"




class SlideLeft : public BC_Radial
{
public:
	SlideLeft(SlideMain *plugin, 
		SlideWindow *window,
		int x,
		int y);
	int handle_event();
	SlideMain *plugin;
	SlideWindow *window;
};

class SlideRight : public BC_Radial
{
public:
	SlideRight(SlideMain *plugin, 
		SlideWindow *window,
		int x,
		int y);
	int handle_event();
	SlideMain *plugin;
	SlideWindow *window;
};

class SlideIn : public BC_Radial
{
public:
	SlideIn(SlideMain *plugin, 
		SlideWindow *window,
		int x,
		int y);
	int handle_event();
	SlideMain *plugin;
	SlideWindow *window;
};

class SlideOut : public BC_Radial
{
public:
	SlideOut(SlideMain *plugin, 
		SlideWindow *window,
		int x,
		int y);
	int handle_event();
	SlideMain *plugin;
	SlideWindow *window;
};





class SlideWindow : public BC_Window
{
public:
	SlideWindow(SlideMain *plugin, int x, int y);
	void create_objects();
	int close_event();
	SlideMain *plugin;
	SlideLeft *left;
	SlideRight *right;
	SlideIn *in;
	SlideOut *out;
};


PLUGIN_THREAD_HEADER(SlideMain, SlideThread, SlideWindow)


class SlideMain : public PluginVClient
{
public:
	SlideMain(PluginServer *server);
	~SlideMain();

// required for all realtime plugins
	void load_configuration();
	int process_realtime(VFrame *incoming, VFrame *outgoing);
	int load_defaults();
	int save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	int show_gui();
	void raise_window();
	int uses_gui();
	int is_transition();
	int is_video();
	char* plugin_title();
	int set_string();
	VFrame* new_picon();

	int motion_direction;
	int direction;
	SlideThread *thread;
	BC_Hash *defaults;
};

#endif
