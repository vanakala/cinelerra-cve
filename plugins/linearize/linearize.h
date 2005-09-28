#ifndef LINEARIZE_H
#define LINEARIZE_H

class LinearizeEngine;
class LinearizeMain;

#include "linearizewindow.h"
#include "loadbalance.h"
#include "plugincolors.h"
#include "guicast.h"
#include "pluginvclient.h"
#include "thread.h"

#define HISTOGRAM_SIZE 0x10000

class LinearizeConfig
{
public:
	LinearizeConfig();

	int equivalent(LinearizeConfig &that);
	void copy_from(LinearizeConfig &that);
	void interpolate(LinearizeConfig &prev, 
		LinearizeConfig &next, 
		int64_t prev_frame, 
		int64_t next_frame, 
		int64_t current_frame);

	float max;
	float gamma;
	int automatic;
};

class LinearizePackage : public LoadPackage
{
public:
	LinearizePackage();
	int start, end;
};

class LinearizeUnit : public LoadClient
{
public:
	LinearizeUnit(LinearizeMain *plugin);
	void process_package(LoadPackage *package);
	LinearizeMain *plugin;
	int accum[HISTOGRAM_SIZE];
};

class LinearizeEngine : public LoadServer
{
public:
	LinearizeEngine(LinearizeMain *plugin);

	void process_packages(int operation, VFrame *data);
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
	VFrame *data;
	int operation;
	enum
	{
		HISTOGRAM,
		APPLY
	};
	LinearizeMain *plugin;
	int accum[HISTOGRAM_SIZE];
};

class LinearizeMain : public PluginVClient
{
public:
	LinearizeMain(PluginServer *server);
	~LinearizeMain();

// required for all realtime plugins
	int process_realtime(VFrame *input_ptr, VFrame *output_ptr);
	void calculate_max(VFrame *frame);
	int is_realtime();
	void update_gui();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	int load_defaults();
	int save_defaults();
	void render_gui(void *data);

	LinearizeEngine *engine;
	VFrame *frame;

	PLUGIN_CLASS_MEMBERS(LinearizeConfig, LinearizeThread)
};



#endif
