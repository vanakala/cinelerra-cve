#ifndef THRESHOLD_H
#define THRESHOLD_H


#include "histogramengine.inc"
#include "loadbalance.h"
#include "thresholdwindow.inc"
#include "plugincolors.inc"
#include "pluginvclient.h"


class ThresholdEngine;

class ThresholdConfig
{
public:
	ThresholdConfig();
	int equivalent(ThresholdConfig &that);
	void copy_from(ThresholdConfig &that);
	void interpolate(ThresholdConfig &prev,
		ThresholdConfig &next,
		int64_t prev_frame, 
		int64_t next_frame, 
		int64_t current_frame);
	void reset();
	void boundaries();

	float min;
	float max;
};



class ThresholdMain : public PluginVClient
{
public:
	ThresholdMain(PluginServer *server);
	~ThresholdMain();

	int process_buffer(VFrame *frame,
		int64_t start_position,
		double frame_rate);
	int is_realtime();
	int load_defaults();
	int save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void update_gui();
	void render_gui(void *data);
	void calculate_histogram(VFrame *frame);

	PLUGIN_CLASS_MEMBERS(ThresholdConfig, ThresholdThread);
	HistogramEngine *engine;
	ThresholdEngine *threshold_engine;
};






class ThresholdPackage : public LoadPackage
{
public:
	ThresholdPackage();
	int start;
	int end;
};


class ThresholdUnit : public LoadClient
{
public:
	ThresholdUnit(ThresholdEngine *server);
	void process_package(LoadPackage *package);
	
	ThresholdEngine *server;
};


class ThresholdEngine : public LoadServer
{
public:
	ThresholdEngine(ThresholdMain *plugin);
	~ThresholdEngine();

	void process_packages(VFrame *data);
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();

	YUV *yuv;
	ThresholdMain *plugin;
	VFrame *data;
};






#endif
