#ifndef DIFF_KEY_H
#define DIFF_KEY_H

#define ftype float

class DiffKeyMain;

#include "filexml.h"
#include "diffkeywindow.h"
#include "guicast.h"
#include "pluginvclient.h"

class DiffKeyConfig
{
public:
	DiffKeyConfig();
	void copy_from(DiffKeyConfig &that);
	int equivalent(DiffKeyConfig &that);
	void interpolate(DiffKeyConfig &prev, 
		DiffKeyConfig &next, 
		long prev_frame, 
		long next_frame, 
		long current_frame);

	ftype hue_imp;
	ftype sat_imp;
	ftype val_imp;
	ftype r_imp;
	ftype g_imp;
	ftype b_imp;
	ftype vis_thresh;
	ftype trans_thresh;
	ftype desat_thresh;
	int show_mask;
	int hue_on;
	int sat_on;
	int val_on;
	int r_on;
	int g_on;
	int b_on;
	int vis_on;
	int trans_on;
	int desat_on;
};

class DiffKeyMain : public PluginVClient
{
public:
	DiffKeyMain(PluginServer *server);
	~DiffKeyMain();

	PLUGIN_CLASS_MEMBERS(DiffKeyConfig, DiffKeyThread);

// required for all realtime plugins
	int process_realtime(VFrame *input_ptr, VFrame *output_ptr);
	int is_realtime();
	void update_gui();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	int load_defaults();
	int save_defaults();
	VFrame *key_frame;
};


#endif
