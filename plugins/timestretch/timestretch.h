#ifndef TIMESTRETCH_H
#define TIMESTRETCH_H

#include "defaults.inc"
#include <fourier.h>
#include "guicast.h"
#include "mainprogress.inc"
#include "pluginaclient.h"
#include "resample.inc"
#include "vframe.inc"




class TimeStretch;



class TimeStretchFraction : public BC_TextBox
{
public:
	TimeStretchFraction(TimeStretch *plugin, int x, int y);
	int handle_event();
	TimeStretch *plugin;
};



class TimeStretchWindow : public BC_Window
{
public:
	TimeStretchWindow(TimeStretch *plugin, int x, int y);
	~TimeStretchWindow();

	void create_objects();

	TimeStretch *plugin;
};


class PitchEngine : public CrossfadeFFT
{
public:
	PitchEngine(TimeStretch *plugin);


	int signal_process();

	TimeStretch *plugin;
};



class TimeStretch : public PluginAClient
{
public:
	TimeStretch(PluginServer *server);
	~TimeStretch();
	
	
	char* plugin_title();
	int get_parameters();
	VFrame* new_picon();
	int start_loop();
	int process_loop(double *buffer, long &write_length);
	int stop_loop();
	
	int load_defaults();
	int save_defaults();
	
	
	

	PitchEngine *pitch;
	Resample *resample;
	double *temp;
	int temp_allocated;

	Defaults *defaults;
	MainProgressBar *progress;
	double scale;
	long current_position;
	long total_written;
};


#endif
