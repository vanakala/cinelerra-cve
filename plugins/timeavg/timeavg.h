#ifndef TIMEAVG_H
#define TIMEAVG_H

class TimeAvgMain;

#include "defaults.inc"
#include "pluginvclient.h"
#include "timeavgwindow.h"
#include "vframe.inc"

class TimeAvgConfig
{
public:
	TimeAvgConfig();
	void copy_from(TimeAvgConfig *src);
	int equivalent(TimeAvgConfig *src);

	int frames;
	int accumulate;
	int paranoid;
};


class TimeAvgMain : public PluginVClient
{
public:
	TimeAvgMain(PluginServer *server);
	~TimeAvgMain();

// required for all realtime plugins
	int process_buffer(VFrame *frame,
		int64_t start_position,
		double frame_rate);
	int is_realtime();
	char* plugin_title();
	VFrame* new_picon();
	int show_gui();
	int load_configuration();
	int set_string();
	int load_defaults();
	int save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void raise_window();
	void update_gui();
	void clear_accum(int w, int h, int color_model);
	void subtract_accum(VFrame *frame);
	void add_accum(VFrame *frame);
	void transfer_accum(VFrame *frame);


	VFrame **history;
// Frame of history in requested samplerate
	int64_t *history_frame;
	int *history_valid;
	unsigned char *accumulation;

// a thread for the GUI
	TimeAvgThread *thread;
	TimeAvgConfig config;
	int history_size;
// Starting frame of history in requested framerate
	int64_t history_start;

	Defaults *defaults;
};


#endif
