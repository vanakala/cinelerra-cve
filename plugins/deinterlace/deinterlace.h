#ifndef DEINTERLACE_H
#define DEINTERLACE_H

// the simplest plugin possible

class DeInterlaceMain;

#include "defaults.inc"
#include "deinterwindow.h"
#include "pluginvclient.h"
#include "vframe.inc"



#define THRESHOLD_SCALAR 1000

enum
{
	DEINTERLACE_NONE,
	DEINTERLACE_EVEN,
	DEINTERLACE_ODD,
	DEINTERLACE_AVG,
	DEINTERLACE_SWAP,
	DEINTERLACE_AVG_ODD,
	DEINTERLACE_AVG_EVEN
};

class DeInterlaceConfig
{
public:
	DeInterlaceConfig();

	int equivalent(DeInterlaceConfig &that);
	void copy_from(DeInterlaceConfig &that);
	void interpolate(DeInterlaceConfig &prev, 
		DeInterlaceConfig &next, 
		long prev_frame, 
		long next_frame, 
		long current_frame);

	int mode;
	int adaptive;
	int threshold;
};

class DeInterlaceMain : public PluginVClient
{
public:
	DeInterlaceMain(PluginServer *server);
	~DeInterlaceMain();


	PLUGIN_CLASS_MEMBERS(DeInterlaceConfig, DeInterlaceThread)
	

// required for all realtime plugins
	int process_realtime(VFrame *input, VFrame *output);
	int is_realtime();
	int hide_gui();
	void update_gui();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	int load_defaults();
	int save_defaults();
	void render_gui(void *data);

	void deinterlace_even(VFrame *input, VFrame *output, int dominance);
	void deinterlace_avg_even(VFrame *input, VFrame *output, int dominance);
	void deinterlace_avg(VFrame *input, VFrame *output);
	void deinterlace_swap(VFrame *input, VFrame *output);

	int changed_rows;
	VFrame *temp;
};


#endif
