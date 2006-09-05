#ifndef INTERPOLATEPIXELS_H
#define INTERPOLATEPIXELS_H

class InterpolatePixelsMain;

#define TOTAL_PATTERNS 2

#include "bchash.h"
#include "loadbalance.h"
#include "mutex.h"
#include "pluginvclient.h"
#include <sys/types.h>

class InterpolatePixelsConfig
{
public:
	InterpolatePixelsConfig();
    int equivalent(InterpolatePixelsConfig &that);
    void copy_from(InterpolatePixelsConfig &that);
    void interpolate(InterpolatePixelsConfig &prev,
     	InterpolatePixelsConfig &next,
     	int64_t prev_position,
     	int64_t next_position,
     	int64_t current_position);
	int x, y;
};

class InterpolatePixelsThread;
class InterpolatePixelsWindow;
class InterpolatePixelsEngine;

class InterpolatePixelsOffset : public BC_ISlider
{
public:
	InterpolatePixelsOffset(InterpolatePixelsWindow *window, 
		int x, 
		int y, 
		int *output);
	~InterpolatePixelsOffset();
	
	int handle_event();
	InterpolatePixelsWindow *window;
	int *output;
};

class InterpolatePixelsWindow : public BC_Window
{
public:
	InterpolatePixelsWindow(InterpolatePixelsMain *client, int x, int y);
	~InterpolatePixelsWindow();
	
	int create_objects();
	int close_event();

	InterpolatePixelsMain *client;
	InterpolatePixelsOffset *x_offset;
	InterpolatePixelsOffset *y_offset;
};

PLUGIN_THREAD_HEADER(InterpolatePixelsMain, 
	InterpolatePixelsThread, 
	InterpolatePixelsWindow);

class InterpolatePixelsMain : public PluginVClient
{
public:
	InterpolatePixelsMain(PluginServer *server);
	~InterpolatePixelsMain();

	PLUGIN_CLASS_MEMBERS(InterpolatePixelsConfig, InterpolatePixelsThread);

// required for all realtime plugins
	int process_buffer(VFrame *frame,
		int64_t start_position,
		double frame_rate);
	int is_realtime();
	void update_gui();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	int load_defaults();
	int save_defaults();
	int handle_opengl();
	
	InterpolatePixelsEngine *engine;
};


class InterpolatePixelsPackage : public LoadPackage
{
public:
	InterpolatePixelsPackage();
	int y1, y2;
};

class InterpolatePixelsUnit : public LoadClient
{
public:
	InterpolatePixelsUnit(InterpolatePixelsEngine *server, InterpolatePixelsMain *plugin);
	void process_package(LoadPackage *package);
	InterpolatePixelsEngine *server;
	InterpolatePixelsMain *plugin;
};

class InterpolatePixelsEngine : public LoadServer
{
public:
	InterpolatePixelsEngine(InterpolatePixelsMain *plugin);
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
	InterpolatePixelsMain *plugin;
	float color_matrix[9];
};


#endif
