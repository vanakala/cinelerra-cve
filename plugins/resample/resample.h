#ifndef RESAMPLEEFFECT_H
#define RESAMPLEEFFECT_H


#include "defaults.inc"
#include "guicast.h"
#include "mainprogress.inc"
#include "pluginaclient.h"
#include "resample.inc"
#include "vframe.inc"

class ResampleEffect;


class ResampleFraction : public BC_TextBox
{
public:
	ResampleFraction(ResampleEffect *plugin, int x, int y);
	int handle_event();
	ResampleEffect *plugin;
};



class ResampleWindow : public BC_Window
{
public:
	ResampleWindow(ResampleEffect *plugin, int x, int y);
	void create_objects();
	ResampleEffect *plugin;
};



class ResampleEffect : public PluginAClient
{
public:
	ResampleEffect(PluginServer *server);
	~ResampleEffect();

	char* plugin_title();
	int get_parameters();
	VFrame* new_picon();
	int start_loop();
	int process_loop(double *buffer, int64_t &write_length);
	int stop_loop();
	int load_defaults();
	int save_defaults();
	void reset();


	Resample *resample;
	double scale;
	Defaults *defaults;
	MainProgressBar *progress;
	int64_t total_written;
	int64_t current_position;
};





#endif
