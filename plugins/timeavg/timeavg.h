#ifndef TIMEAVG_H
#define TIMEAVG_H

class TimeAvgMain;

#include "defaults.inc"
#include "pluginvclient.h"
#include "timeavgwindow.h"
#include "vframe.inc"

class TimeAvgConfig
{
public:
	TimeAvgConfig();
	
	int frames;
};


class TimeAvgMain : public PluginVClient
{
public:
	TimeAvgMain(PluginServer *server);
	~TimeAvgMain();

// required for all realtime plugins
	int process_realtime(VFrame *input, VFrame *output);
	int is_realtime();
	char* plugin_title();
	VFrame* new_picon();
	int show_gui();
	void load_configuration();
	int set_string();
	int load_defaults();
	int save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void raise_window();
	void update_gui();


	VFrame **history;
	int *accumulation;

// a thread for the GUI
	TimeAvgThread *thread;
	TimeAvgConfig config;
	int history_size;

	Defaults *defaults;
};


#endif
