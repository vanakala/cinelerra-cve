#ifndef REVERB_H
#define REVERB_H

class Reverb;
class ReverbEngine;

#include "reverbwindow.h"
#include "pluginaclient.h"

class ReverbConfig
{
public:
	ReverbConfig();


	int equivalent(ReverbConfig &that);
	void copy_from(ReverbConfig &that);
	void interpolate(ReverbConfig &prev, 
		ReverbConfig &next, 
		long prev_frame, 
		long next_frame, 
		long current_frame);
	void dump();

	double level_init;
	long delay_init;
	double ref_level1;
	double ref_level2;
	long ref_total;
	long ref_length;
	long lowpass1, lowpass2;
};

class Reverb : public PluginAClient
{
public:
	Reverb(PluginServer *server);
	~Reverb();

	void update_gui();
	int load_from_file(char *data);
	int save_to_file(char *data);
	int load_configuration();
	
// data for reverb
	ReverbConfig config;
	
	char config_directory[1024];

	double **main_in, **main_out;
	double **dsp_in;
	long **ref_channels, **ref_offsets, **ref_lowpass;
	double **ref_levels;
	long dsp_in_length;
	int redo_buffers;
// skirts for lowpass filter
	double **lowpass_in1, **lowpass_in2;
	DB db;
// required for all realtime/multichannel plugins

	int process_realtime(long size, double **input_ptr, double **output_ptr);
	int is_realtime();
	int is_multichannel();
	char* plugin_title();
	int show_gui();
	int set_string();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void raise_window();
	VFrame* new_picon();

// non realtime support
	int load_defaults();
	int save_defaults();
	Defaults *defaults;
	
	ReverbThread *thread;
	ReverbEngine **engine;
	int initialized;
};

class ReverbEngine : public Thread
{
public:
	ReverbEngine(Reverb *plugin);
	~ReverbEngine();

	int process_overlay(double *in, double *out, double &out1, double &out2, double level, long lowpass, long samplerate, long size);
	int process_overlays(int output_buffer, long size);
	int wait_process_overlays();
	void run();

	Mutex input_lock, output_lock;
	int completed;
	int output_buffer;
	long size;
	Reverb *plugin;
};

#endif
