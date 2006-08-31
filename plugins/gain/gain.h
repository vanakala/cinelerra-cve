#ifndef GAIN_H
#define GAIN_H

class Gain;
class GainEngine;

#include "gainwindow.h"
#include "pluginaclient.h"

class GainConfig
{
public:
	GainConfig();
	int equivalent(GainConfig &that);
	void copy_from(GainConfig &that);
	void interpolate(GainConfig &prev, 
		GainConfig &next, 
		int64_t prev_frame, 
		int64_t next_frame, 
		int64_t current_frame);

	double level;
};

class Gain : public PluginAClient
{
public:
	Gain(PluginServer *server);
	~Gain();

	int process_realtime(int64_t size, double *input_ptr, double *output_ptr);

	PLUGIN_CLASS_MEMBERS(GainConfig, GainThread)
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	int load_defaults();
	int save_defaults();
	void update_gui();
	int is_realtime();


	DB db;
};

#endif
