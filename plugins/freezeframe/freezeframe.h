#ifndef FREEZEFRAME_H
#define FREEZEFRAME_H




#include "filexml.inc"
#include "mutex.h"
#include "pluginvclient.h"



class FreezeFrameWindow;
class FreezeFrameMain;
class FreezeFrameThread;

class FreezeFrameConfig
{
public:
	FreezeFrameConfig();
	void copy_from(FreezeFrameConfig &that);
	int equivalent(FreezeFrameConfig &that);
	void interpolate(FreezeFrameConfig &prev, 
		FreezeFrameConfig &next, 
		long prev_frame, 
		long next_frame, 
		long current_frame);
	int enabled;
};

class FreezeFrameToggle : public BC_CheckBox
{
public:
	FreezeFrameToggle(FreezeFrameMain *client, int x, int y);
	~FreezeFrameToggle();
	int handle_event();
	FreezeFrameMain *client;
};

class FreezeFrameWindow : public BC_Window
{
public:
	FreezeFrameWindow(FreezeFrameMain *client, int x, int y);
	~FreezeFrameWindow();
	
	int create_objects();
	int close_event();
	
	FreezeFrameMain *client;
	FreezeFrameToggle *enabled;
};

PLUGIN_THREAD_HEADER(FreezeFrameMain, FreezeFrameThread, FreezeFrameWindow)

class FreezeFrameMain : public PluginVClient
{
public:
	FreezeFrameMain(PluginServer *server);
	~FreezeFrameMain();

// required for all realtime plugins
	int process_realtime(VFrame *input_ptr, VFrame *output_ptr);
	int is_realtime();
	char* plugin_title();
	int show_gui();
	void raise_window();
	int set_string();
	void update_gui();
	int load_configuration();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	int load_defaults();
	int save_defaults();
	VFrame* new_picon();
	int is_synthesis();

// parameters needed for freezeframe
	VFrame *first_frame;
	FreezeFrameConfig config;
	FreezeFrameThread *thread;
	Defaults *defaults;
};


#endif
