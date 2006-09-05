#ifndef FLIP_H
#define FLIP_H


class FlipMain;

#include "filexml.h"
#include "flipwindow.h"
#include "guicast.h"
#include "pluginvclient.h"

class FlipConfig
{
public:
	FlipConfig();
	void copy_from(FlipConfig &that);
	int equivalent(FlipConfig &that);
	void interpolate(FlipConfig &prev, 
		FlipConfig &next, 
		long prev_frame, 
		long next_frame, 
		long current_frame);
	int flip_horizontal;
	int flip_vertical;
};

class FlipMain : public PluginVClient
{
public:
	FlipMain(PluginServer *server);
	~FlipMain();

	PLUGIN_CLASS_MEMBERS(FlipConfig, FlipThread);

// required for all realtime plugins
	int process_buffer(VFrame *frame,
		int64_t start_position,
		double frame_rate);
	int is_realtime();
	void update_gui();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	int load_defaults();
	int save_defaults();
	int handle_opengl();
};


#endif
