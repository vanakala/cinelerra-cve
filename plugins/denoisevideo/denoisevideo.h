#ifndef DENOISEVIDEO_H
#define DENOISEVIDEO_H

class DenoiseVideo;
class DenoiseVideoWindow;

#include "bcdisplayinfo.h"
#include "defaults.inc"
#include "pluginvclient.h"
#include "vframe.inc"



class DenoiseVideoConfig
{
public:
	DenoiseVideoConfig();

	int equivalent(DenoiseVideoConfig &that);
	void copy_from(DenoiseVideoConfig &that);
	void interpolate(DenoiseVideoConfig &prev, 
		DenoiseVideoConfig &next, 
		long prev_frame, 
		long next_frame, 
		long current_frame);

	int frames;
	float threshold;
	int do_r, do_g, do_b, do_a;
};




class DenoiseVideoFrames : public BC_ISlider
{
public:
	DenoiseVideoFrames(DenoiseVideo *plugin, int x, int y);
	int handle_event();
	DenoiseVideo *plugin;
};


class DenoiseVideoThreshold : public BC_TextBox
{
public:
	DenoiseVideoThreshold(DenoiseVideo *plugin, int x, int y);
	int handle_event();
	DenoiseVideo *plugin;
};

class DenoiseVideoToggle : public BC_CheckBox
{
public:
	DenoiseVideoToggle(DenoiseVideo *plugin, 
		DenoiseVideoWindow *gui, 
		int x, 
		int y, 
		int *output,
		char *text);
	int handle_event();
	DenoiseVideo *plugin;
	int *output;
};


class DenoiseVideoWindow : public BC_Window
{
public:
	DenoiseVideoWindow(DenoiseVideo *plugin, int x, int y);

	void create_objects();
	int close_event();
	
	DenoiseVideo *plugin;
	DenoiseVideoFrames *frames;
	DenoiseVideoThreshold *threshold;
	DenoiseVideoToggle *do_r, *do_g, *do_b, *do_a;
};


PLUGIN_THREAD_HEADER(DenoiseVideo, DenoiseVideoThread, DenoiseVideoWindow)

class DenoiseVideo : public PluginVClient
{
public:
	DenoiseVideo(PluginServer *server);
	~DenoiseVideo();

	int process_realtime(VFrame *input, VFrame *output);
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

	float *accumulation;
	DenoiseVideoThread *thread;
	DenoiseVideoConfig config;
	Defaults *defaults;
};



#endif
