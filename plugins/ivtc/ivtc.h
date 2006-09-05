#ifndef IVTC_H
#define IVTC_H

class IVTCMain;
class IVTCEngine;

#include "bchash.h"
#include "loadbalance.h"
#include "mutex.h"
#include "pluginvclient.h"
#include "ivtcwindow.h"
#include <sys/types.h>

class IVTCEngine;

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
	enum
	{
		PULLDOWN32,
		SHIFTFIELD,
		AUTOMATIC
	};
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
	void render_stop();

	int load_defaults();
	int save_defaults();

	void compare_fields(VFrame *frame1, 
		VFrame *frame2, 
		int64_t &field1,
		int64_t &field2);
	int64_t compare(VFrame *current_avg, 
		VFrame *current_orig, 
		VFrame *previous, 
		int field);

	void deinterlace_avg(VFrame *output, VFrame *input, int dominance);


	VFrame *temp_frame[2];
	VFrame *input, *output;

// Automatic IVTC variables
// Difference between averaged current even lines and original even lines
	int64_t even_vs_current;
// Difference between averaged current even lines and previous even lines
	int64_t even_vs_prev;
// Difference between averaged current odd lines and original odd lines
	int64_t odd_vs_current;
// Difference between averaged current odd lines and previous odd lines
	int64_t odd_vs_prev;

// Closest combination of fields in previous calculation.
// If the lowest current combination is too big and the previous strategy
// was direct copy, copy the previous frame.
	int64_t previous_min;
	int previous_strategy;
	IVTCEngine *engine;
};


class IVTCPackage : public LoadPackage
{
public:
	IVTCPackage();
	int y1, y2;
};

class IVTCUnit : public LoadClient
{
public:
	IVTCUnit(IVTCEngine *server, IVTCMain *plugin);
	void process_package(LoadPackage *package);
	void clear_totals();
	IVTCEngine *server;
	IVTCMain *plugin;
	int64_t even_vs_current;
	int64_t even_vs_prev;
	int64_t odd_vs_current;
	int64_t odd_vs_prev;
};


class IVTCEngine : public LoadServer
{
public:
	IVTCEngine(IVTCMain *plugin, int cpus);
	~IVTCEngine();
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
	IVTCMain *plugin;
};

#endif
