#ifndef DESPIKE_H
#define DESPIKE_H

class Despike;
class DespikeEngine;

#include "despikewindow.h"
#include "pluginaclient.h"

class DespikeConfig
{
public:
	DespikeConfig();

	int equivalent(DespikeConfig &that);
	void copy_from(DespikeConfig &that);
	void interpolate(DespikeConfig &prev, 
		DespikeConfig &next, 
		int64_t prev_frame, 
		int64_t next_frame, 
		int64_t current_frame);

	double level;
	double slope;
};

class Despike : public PluginAClient
{
public:
	Despike(PluginServer *server);
	~Despike();

	void update_gui();
	int load_configuration();
	
// data for despike
	DespikeConfig config;
	
	DB db;

	char* plugin_title();
	VFrame* new_picon();
	int is_realtime();
	int process_realtime(int64_t size, double *input_ptr, double *output_ptr);
	int show_gui();
	int set_string();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void raise_window();

// non realtime support
	int load_defaults();
	int save_defaults();
	Defaults *defaults;
	
	DespikeThread *thread;
	double last_sample;
};

#endif
