#ifndef TIMESTRETCH_H
#define TIMESTRETCH_H

#include "bchash.inc"
#include "fourier.h"
#include "guicast.h"
#include "mainprogress.inc"
#include "pluginaclient.h"
#include "resample.inc"
#include "timestretchengine.inc"
#include "vframe.inc"




class TimeStretch;
class TimeStretchWindow;
class TimeStretchConfig;



class TimeStretchScale : public BC_FPot
{
public:
	TimeStretchScale(TimeStretch *plugin, int x, int y);
	int handle_event();
	TimeStretch *plugin;
};

class TimeStretchWindow : public BC_Window
{
public:
	TimeStretchWindow(TimeStretch *plugin, int x, int y);
	void create_objects();
	void update();
	int close_event();
	TimeStretchScale *scale;
	TimeStretch *plugin;
};

PLUGIN_THREAD_HEADER(TimeStretch, TimeStretchThread, TimeStretchWindow)


class TimeStretchConfig
{
public:
	TimeStretchConfig();


	int equivalent(TimeStretchConfig &that);
	void copy_from(TimeStretchConfig &that);
	void interpolate(TimeStretchConfig &prev, 
		TimeStretchConfig &next, 
		int64_t prev_frame, 
		int64_t next_frame, 
		int64_t current_frame);


	double scale;
};



class PitchEngine : public CrossfadeFFT
{
public:
	PitchEngine(TimeStretch *plugin);
	~PitchEngine();


	int read_samples(int64_t output_sample, 
		int samples, 
		double *buffer);
	int signal_process_oversample(int reset);

	TimeStretch *plugin;
	double *temp;
	double *input_buffer;
	int input_size;
	int input_allocated;
	int64_t current_input_sample;
	int64_t current_output_sample;

	double *last_phase;
	double *new_freq;
	double *new_magn;
	double *sum_phase;
	double *anal_freq;
	double *anal_magn;


};

class TimeStretch : public PluginAClient
{
public:
	TimeStretch(PluginServer *server);
	~TimeStretch();
	
	
	VFrame* new_picon();
	char* plugin_title();
	int is_realtime();
	int get_parameters();
	void read_data(KeyFrame *keyframe);
	void save_data(KeyFrame *keyframe);

	int process_buffer(int64_t size, 
		double *buffer,
		int64_t start_position,
		int sample_rate);


	int show_gui();
	void raise_window();
	int set_string();

	
	int load_configuration();
	int load_defaults();
	int save_defaults();
	
	void update_gui();
	
	

	PitchEngine *pitch;
	Resample *resample;
	double *temp;
	int temp_allocated;
	double *input;
	int input_allocated;

	TimeStretchEngine *stretch;

	BC_Hash *defaults;
	TimeStretchConfig config;
	TimeStretchThread *thread;

};


#endif
