#ifndef PITCH_H
#define PITCH_H



#include "defaults.inc"
#include "../fourier/fourier.h"
#include "guicast.h"
#include "mutex.h"
#include "pluginaclient.h"
#include "vframe.inc"

class PitchEffect;

class PitchScale : public BC_FPot
{
public:
	PitchScale(PitchEffect *plugin, int x, int y);
	int handle_event();
	PitchEffect *plugin;
};

class PitchWindow : public BC_Window
{
public:
	PitchWindow(PitchEffect *plugin, int x, int y);
	void create_objects();
	void update();
	int close_event();
	PitchScale *scale;
	PitchEffect *plugin;
};

class PitchThread : public Thread
{
public:
	PitchThread(PitchEffect *plugin);
	~PitchThread();
	void run();
	Mutex completion;
	PitchWindow *window;
	PitchEffect *plugin;
};

class PitchConfig
{
public:
	PitchConfig();


	int equivalent(PitchConfig &that);
	void copy_from(PitchConfig &that);
	void interpolate(PitchConfig &prev, 
		PitchConfig &next, 
		long prev_frame, 
		long next_frame, 
		long current_frame);


	double scale;
};

class PitchFFT : public CrossfadeFFT
{
public:
	PitchFFT(PitchEffect *plugin);
	int signal_process();
	PitchEffect *plugin;
};

class PitchEffect : public PluginAClient
{
public:
	PitchEffect(PluginServer *server);
	~PitchEffect();

	VFrame* new_picon();
	char* plugin_title();
	int is_realtime();
	void read_data(KeyFrame *keyframe);
	void save_data(KeyFrame *keyframe);
	int process_realtime(long size, double *input_ptr, double *output_ptr);
	int show_gui();
	void raise_window();
	int set_string();




	int load_defaults();
	int save_defaults();
	int load_configuration();
	void reset();
	void update_gui();


	Defaults *defaults;
	PitchThread *thread;
	PitchFFT *fft;
	PitchConfig config;
};


#endif
