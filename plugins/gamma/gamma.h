#ifndef LINEARIZE_H
#define LINEARIZE_H

class GammaEngine;
class GammaMain;

#include "gammawindow.h"
#include "loadbalance.h"
#include "plugincolors.h"
#include "guicast.h"
#include "pluginvclient.h"
#include "thread.h"

#define HISTOGRAM_SIZE 256

class GammaConfig
{
public:
	GammaConfig();

	int equivalent(GammaConfig &that);
	void copy_from(GammaConfig &that);
	void interpolate(GammaConfig &prev, 
		GammaConfig &next, 
		int64_t prev_frame, 
		int64_t next_frame, 
		int64_t current_frame);

	float max;
	float gamma;
	int automatic;
// Plot histogram
	int plot;
};

class GammaPackage : public LoadPackage
{
public:
	GammaPackage();
	int start, end;
};

class GammaUnit : public LoadClient
{
public:
	GammaUnit(GammaMain *plugin);
	void process_package(LoadPackage *package);
	GammaMain *plugin;
	int accum[HISTOGRAM_SIZE];
};

class GammaEngine : public LoadServer
{
public:
	GammaEngine(GammaMain *plugin);

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
	GammaMain *plugin;
	int accum[HISTOGRAM_SIZE];
};

class GammaMain : public PluginVClient
{
public:
	GammaMain(PluginServer *server);
	~GammaMain();

// required for all realtime plugins
	int process_buffer(VFrame *frame,
		int64_t start_position,
		double frame_rate);
	void calculate_max(VFrame *frame);
	int is_realtime();
	void update_gui();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	int load_defaults();
	int save_defaults();
	void render_gui(void *data);
	int handle_opengl();

	GammaEngine *engine;
	VFrame *frame;

	PLUGIN_CLASS_MEMBERS(GammaConfig, GammaThread)
};



#endif
