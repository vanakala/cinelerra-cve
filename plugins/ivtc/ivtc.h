#ifndef IVTC_H
#define IVTC_H

class IVTCMain;
class IVTCEngine;

#define TOTAL_PATTERNS 2

#include "defaults.h"
#include "mutex.h"
#include "pluginvclient.h"
#include "ivtcwindow.h"
#include <sys/types.h>

class IVTCConfig
{
public:
	IVTCConfig();
	int frame_offset;
// 0 - even     1 - odd
	int first_field;
	int automatic;
	float auto_threshold;
	int pattern;
};

class IVTCMain : public PluginVClient
{
public:
	IVTCMain(PluginServer *server);
	~IVTCMain();

// required for all realtime plugins
	int process_realtime(VFrame *input_ptr, VFrame *output_ptr);
	int is_realtime();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	PLUGIN_CLASS_MEMBERS(IVTCConfig, IVTCThread)
	void update_gui();

	int load_defaults();
	int save_defaults();

	void compare_fields(VFrame *frame1, 
		VFrame *frame2, 
		int64_t &field1,
		int64_t &field2);


// 0, 3, 4 indicating pattern frame the automatic algorithm is on
	int state;
// New field detected in state 2
	int new_field;
	int64_t average, total_average;
	VFrame *temp_frame[2];

	IVTCEngine **engine;
};

class IVTCEngine : public Thread
{
public:
	IVTCEngine(IVTCMain *plugin, int start_y, int end_y);
	~IVTCEngine();
	
	void run();
	int start_process_frame(VFrame *output, VFrame *input);
	int wait_process_frame();

	IVTCMain *plugin;
	int start_y, end_y;
	VFrame *output, *input;
	Mutex input_lock, output_lock;
	int last_frame;
	int64_t field1, field2;
};

#endif
