#ifndef BRIGHTNESS_H
#define BRIGHTNESS_H

class BrightnessEngine;
class BrightnessMain;

#include "brightnesswindow.h"
#include "loadbalance.h"
#include "plugincolors.h"
#include "pluginvclient.h"

class BrightnessConfig
{
public:
	BrightnessConfig();

	int equivalent(BrightnessConfig &that);
	void copy_from(BrightnessConfig &that);
	void interpolate(BrightnessConfig &prev, 
		BrightnessConfig &next, 
		int64_t prev_frame, 
		int64_t next_frame, 
		int64_t current_frame);

	float brightness;
	float contrast;
	int luma;
};

class BrightnessMain : public PluginVClient
{
public:
	BrightnessMain(PluginServer *server);
	~BrightnessMain();

// required for all realtime plugins
	int process_buffer(VFrame *frame,
		int64_t start_position,
		double frame_rate);
	int is_realtime();
	char* plugin_title();
	int show_gui();
	void raise_window();
	void update_gui();
	int set_string();
	int load_configuration();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	int load_defaults();
	int save_defaults();
	VFrame* new_picon();
	int handle_opengl();








	BrightnessConfig config;
// a thread for the GUI
	BrightnessThread *thread;
	BrightnessEngine *engine;
	BC_Hash *defaults;
    int redo_buffers;
	static YUV yuv;
	
	VFrame *input, *output;
};






class BrightnessPackage : public LoadPackage
{
public:
	BrightnessPackage();
	
	int row1, row2;
};

class BrightnessUnit : public LoadClient
{
public:
	BrightnessUnit(BrightnessEngine *server, BrightnessMain *plugin);
	~BrightnessUnit();
	
	void process_package(LoadPackage *package);
	
	BrightnessMain *plugin;
};

class BrightnessEngine : public LoadServer
{
public:
	BrightnessEngine(BrightnessMain *plugin, int cpus);
	~BrightnessEngine();

	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
	
	BrightnessMain *plugin;
};









#endif
