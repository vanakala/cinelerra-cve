#ifndef IRISSQUARE_H
#define IRISSQUARE_H

class IrisSquareMain;
class IrisSquareWindow;


#include "overlayframe.inc"
#include "pluginvclient.h"
#include "vframe.inc"




class IrisSquareIn : public BC_Radial
{
public:
	IrisSquareIn(IrisSquareMain *plugin, 
		IrisSquareWindow *window,
		int x,
		int y);
	int handle_event();
	IrisSquareMain *plugin;
	IrisSquareWindow *window;
};

class IrisSquareOut : public BC_Radial
{
public:
	IrisSquareOut(IrisSquareMain *plugin, 
		IrisSquareWindow *window,
		int x,
		int y);
	int handle_event();
	IrisSquareMain *plugin;
	IrisSquareWindow *window;
};




class IrisSquareWindow : public BC_Window
{
public:
	IrisSquareWindow(IrisSquareMain *plugin, int x, int y);
	void create_objects();
	int close_event();
	IrisSquareMain *plugin;
	IrisSquareIn *in;
	IrisSquareOut *out;
};


PLUGIN_THREAD_HEADER(IrisSquareMain, IrisSquareThread, IrisSquareWindow)


class IrisSquareMain : public PluginVClient
{
public:
	IrisSquareMain(PluginServer *server);
	~IrisSquareMain();

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
	IrisSquareThread *thread;
	BC_Hash *defaults;
};

#endif
