#ifndef FLIP_H
#define FLIP_H

// the simplest plugin possible

class InvertMain;

#include "bcbase.h"
#include "invertwindow.h"
#include "pluginvclient.h"


class InvertMain : public PluginVClient
{
public:
	InvertMain(int argc, char *argv[]);
	~InvertMain();

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
	int save_data(char *text);
	int read_data(char *text);

// parameters needed for invert
	int invert;

// a thread for the GUI
	InvertThread *thread;

private:
// Utilities used by invert.
};


#endif
