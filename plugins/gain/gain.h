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
		long prev_frame, 
		long next_frame, 
		long current_frame);

	double level;
};

class Gain : public PluginAClient
{
public:
	Gain(PluginServer *server);
	~Gain();

	int process_realtime(long size, double *input_ptr, double *output_ptr);

	PLUGIN_CLASS_MEMBERS(GainConfig, GainThread)
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	int load_defaults();
	int save_defaults();
	void update_gui();
	int is_realtime();


	DB db;
};

class GainEngine : public Thread
{
public:
	GainEngine(Gain *plugin);
	~GainEngine();

	int process_overlay(double *in, double *out, double &out1, double &out2, double level, long lowpass, long samplerate, long size);
	int process_overlays(int output_buffer, long size);
	int wait_process_overlays();
	void run();

	Mutex input_lock, output_lock;
	int completed;
	int output_buffer;
	long size;
	Gain *plugin;
};

#endif
