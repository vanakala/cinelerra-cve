#ifndef BANDSLIDE_H
#define BANDSLIDE_H

class BandSlideMain;
class BandSlideWindow;


#include "overlayframe.inc"
#include "pluginvclient.h"
#include "vframe.inc"




class BandSlideCount : public BC_TumbleTextBox
{
public:
	BandSlideCount(BandSlideMain *plugin, 
		BandSlideWindow *window,
		int x,
		int y);
	int handle_event();
	BandSlideMain *plugin;
	BandSlideWindow *window;
};

class BandSlideIn : public BC_Radial
{
public:
	BandSlideIn(BandSlideMain *plugin, 
		BandSlideWindow *window,
		int x,
		int y);
	int handle_event();
	BandSlideMain *plugin;
	BandSlideWindow *window;
};

class BandSlideOut : public BC_Radial
{
public:
	BandSlideOut(BandSlideMain *plugin, 
		BandSlideWindow *window,
		int x,
		int y);
	int handle_event();
	BandSlideMain *plugin;
	BandSlideWindow *window;
};




class BandSlideWindow : public BC_Window
{
public:
	BandSlideWindow(BandSlideMain *plugin, int x, int y);
	void create_objects();
	int close_event();
	BandSlideMain *plugin;
	BandSlideCount *count;
	BandSlideIn *in;
	BandSlideOut *out;
};


PLUGIN_THREAD_HEADER(BandSlideMain, BandSlideThread, BandSlideWindow)


class BandSlideMain : public PluginVClient
{
public:
	BandSlideMain(PluginServer *server);
	~BandSlideMain();

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
	BandSlideThread *thread;
	Defaults *defaults;
};

#endif
