#ifndef YUV_H
#define YUV_H

#define MAXVALUE 100

class YUVMain;
class YUVEngine;

#include "bcbase.h"
#include "../colors/colors.h"
#include "yuvwindow.h"
#include "pluginvclient.h"


class YUVMain : public PluginVClient
{
public:
	YUVMain(int argc, char *argv[]);
	~YUVMain();

// required for all realtime plugins
	int process_realtime(long size, VFrame **input_ptr, VFrame **output_ptr);
	int plugin_is_realtime();
	int plugin_is_multi_channel();
	char* plugin_title();
	int start_gui();
	int stop_gui();
	int show_gui();
	int hide_gui();
	int set_string();
	int load_defaults();
	int save_defaults();
	int save_data(char *text);
	int read_data(char *text);

// parameters needed
	int reconfigure();    // Rebuild tables
	int y;
	int u;
	int v;
	int automated_function;
	int reconfigure_flag;

// a thread for the GUI
	YUVThread *thread;

private:
	Defaults *defaults;
	YUVEngine **engine;
};

class YUVEngine : public Thread
{
public:
	YUVEngine(YUVMain *plugin, int start_y, int end_y);
	~YUVEngine();
	
	int start_process_frame(VFrame **output, VFrame **input, int size);
	int wait_process_frame();
	void run();
	
	YUVMain *plugin;
	int start_y;
	int end_y;
	int size;
	VFrame **output, **input;
	int last_frame;
	Mutex input_lock, output_lock;
    YUV yuv;
};


#endif
