#ifndef HUESATURATION_H
#define HUESATURATION_H

#define MAXHUE 180
#define MAXVALUE 100

class HueMain;
class HueEngine;

#include "bcbase.h"
#include "../colors/colors.h"
#include "huewindow.h"
#include "pluginvclient.h"


class HueMain : public PluginVClient
{
public:
	HueMain(int argc, char *argv[]);
	~HueMain();

// required for all realtime plugins
	int process_realtime(long size, VFrame **input_ptr, VFrame **output_ptr);
	int plugin_is_realtime();
	int plugin_is_multi_channel();
	char* plugin_title();
	int start_realtime();
	int stop_realtime();
	int start_gui();
	int stop_gui();
	int show_gui();
	int hide_gui();
	int set_string();
	int load_defaults();
	int save_defaults();
	int save_data(char *text);
	int read_data(char *text);
	void update_gui();

// parameters needed
	int reconfigure();    // Rebuild tables
	int hue;
	int saturation;
	int value;
	int automated_function;
	int reconfigure_flag;

// a thread for the GUI
	HueThread *thread;

private:
	Defaults *defaults;
	HueEngine **engine;
};

class HueEngine : public Thread
{
public:
	HueEngine(HueMain *plugin, int start_y, int end_y);
	~HueEngine();
	
	int start_process_frame(VFrame **output, VFrame **input, int size);
	int wait_process_frame();
	void run();
	
	HueMain *plugin;
	int start_y;
	int end_y;
	int size;
	VFrame **output, **input;
	int last_frame;
	Mutex input_lock, output_lock;
    HSV hsv;
};


#endif
