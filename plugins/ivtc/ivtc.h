#ifndef IVTC_H
#define IVTC_H

class IVTCMain;
class IVTCEngine;

#define TOTAL_PATTERNS 3

static char *pattern_text[] = 
{
	"A  B  BC  CD  D",
	"AB  BC  CD  DE  EF",
	"Automatic"
};

#include "defaults.h"
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
	int64_t even_vs_current;
	int64_t even_vs_prev;
	int64_t odd_vs_current;
	int64_t odd_vs_prev;
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
