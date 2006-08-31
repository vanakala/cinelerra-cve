#ifndef WIPE_H
#define WIPE_H

class WipeMain;
class WipeWindow;


#include "overlayframe.inc"
#include "pluginvclient.h"
#include "vframe.inc"




class WipeLeft : public BC_Radial
{
public:
	WipeLeft(WipeMain *plugin, 
		WipeWindow *window,
		int x,
		int y);
	int handle_event();
	WipeMain *plugin;
	WipeWindow *window;
};

class WipeRight : public BC_Radial
{
public:
	WipeRight(WipeMain *plugin, 
		WipeWindow *window,
		int x,
		int y);
	int handle_event();
	WipeMain *plugin;
	WipeWindow *window;
};




class WipeWindow : public BC_Window
{
public:
	WipeWindow(WipeMain *plugin, int x, int y);
	void create_objects();
	int close_event();
	WipeMain *plugin;
	WipeLeft *left;
	WipeRight *right;
};


PLUGIN_THREAD_HEADER(WipeMain, WipeThread, WipeWindow)


class WipeMain : public PluginVClient
{
public:
	WipeMain(PluginServer *server);
	~WipeMain();

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

	int direction;
	WipeThread *thread;
	BC_Hash *defaults;
};

#endif
