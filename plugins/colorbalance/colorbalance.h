#ifndef COLORBALANCE_H
#define COLORBALANCE_H

class ColorBalanceMain;

#include "colorbalancewindow.h"
#include "plugincolors.h"
#include "guicast.h"
#include "mutex.h"
#include "pluginvclient.h"
#include "thread.h"

#define SHADOWS 0
#define MIDTONES 1
#define HIGHLIGHTS 2

class ColorBalanceConfig
{
public:
	ColorBalanceConfig();

	int equivalent(ColorBalanceConfig &that);
	void copy_from(ColorBalanceConfig &that);
	void interpolate(ColorBalanceConfig &prev, 
		ColorBalanceConfig &next, 
		long prev_frame, 
		long next_frame, 
		long current_frame);

	float cyan;
	float magenta;
    float yellow;
    int preserve;
    int lock_params;
};

class ColorBalanceEngine : public Thread
{
public:
	ColorBalanceEngine(ColorBalanceMain *plugin);
	~ColorBalanceEngine();

	int start_process_frame(VFrame *output, VFrame *input, int row_start, int row_end);
	int wait_process_frame();
	void run();

	ColorBalanceMain *plugin;
	int row_start, row_end;
	int last_frame;
	Mutex input_lock, output_lock;
	VFrame *input, *output;
	YUV yuv;
};

class ColorBalanceMain : public PluginVClient
{
public:
	ColorBalanceMain(PluginServer *server);
	~ColorBalanceMain();

// required for all realtime plugins
	int process_realtime(VFrame *input_ptr, VFrame *output_ptr);
	int is_realtime();
	char* plugin_title();
	int show_gui();
	void raise_window();
	int set_string();
	int load_configuration();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	int load_defaults();
	int save_defaults();
	VFrame* new_picon();

// parameters needed for processor
	int reconfigure();
    int synchronize_params(ColorBalanceSlider *slider, float difference);
    int test_boundary(float &value);

	ColorBalanceConfig config;
// a thread for the GUI
	ColorBalanceThread *thread;
	ColorBalanceEngine **engine;
	int total_engines;


	Defaults *defaults;
    int r_lookup_8[0x100];
    int g_lookup_8[0x100];
    int b_lookup_8[0x100];
    double highlights_add_8[0x100];
    double highlights_sub_8[0x100];
    int r_lookup_16[0x10000];
    int g_lookup_16[0x10000];
    int b_lookup_16[0x10000];
    double highlights_add_16[0x10000];
    double highlights_sub_16[0x10000];
    int redo_buffers;
	int need_reconfigure;
};



#endif
