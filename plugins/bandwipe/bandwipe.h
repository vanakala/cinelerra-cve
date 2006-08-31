#ifndef BANDWIPE_H
#define BANDWIPE_H

class BandWipeMain;
class BandWipeWindow;


#include "overlayframe.inc"
#include "pluginvclient.h"
#include "vframe.inc"




class BandWipeCount : public BC_TumbleTextBox
{
public:
	BandWipeCount(BandWipeMain *plugin, 
		BandWipeWindow *window,
		int x,
		int y);
	int handle_event();
	BandWipeMain *plugin;
	BandWipeWindow *window;
};

class BandWipeIn : public BC_Radial
{
public:
	BandWipeIn(BandWipeMain *plugin, 
		BandWipeWindow *window,
		int x,
		int y);
	int handle_event();
	BandWipeMain *plugin;
	BandWipeWindow *window;
};

class BandWipeOut : public BC_Radial
{
public:
	BandWipeOut(BandWipeMain *plugin, 
		BandWipeWindow *window,
		int x,
		int y);
	int handle_event();
	BandWipeMain *plugin;
	BandWipeWindow *window;
};




class BandWipeWindow : public BC_Window
{
public:
	BandWipeWindow(BandWipeMain *plugin, int x, int y);
	void create_objects();
	int close_event();
	BandWipeMain *plugin;
	BandWipeCount *count;
	BandWipeIn *in;
	BandWipeOut *out;
};


PLUGIN_THREAD_HEADER(BandWipeMain, BandWipeThread, BandWipeWindow)


class BandWipeMain : public PluginVClient
{
public:
	BandWipeMain(PluginServer *server);
	~BandWipeMain();

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

	int bands;
	int direction;
	BandWipeThread *thread;
	BC_Hash *defaults;
};

#endif
