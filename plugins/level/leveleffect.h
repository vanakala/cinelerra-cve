#ifndef SOUNDLEVEL_H
#define SOUNDLEVEL_H

#include "guicast.h"
#include "pluginaclient.h"



class SoundLevelEffect;
class SoundLevelWindow;




class SoundLevelConfig
{
public:
	SoundLevelConfig();
	void copy_from(SoundLevelConfig &that);
	int equivalent(SoundLevelConfig &that);
	void interpolate(SoundLevelConfig &prev, 
		SoundLevelConfig &next, 
		int64_t prev_frame, 
		int64_t next_frame, 
		int64_t current_frame);
	float duration;
};

class SoundLevelDuration : public BC_FSlider
{
public:
	SoundLevelDuration(SoundLevelEffect *plugin, int x, int y);
	int handle_event();
	SoundLevelEffect *plugin;
};

class SoundLevelWindow : public BC_Window
{
public:
	SoundLevelWindow(SoundLevelEffect *plugin, int x, int y);
	void create_objects();
	int close_event();


	BC_Title *soundlevel_max;
	BC_Title *soundlevel_rms;
	SoundLevelDuration *duration;
	SoundLevelEffect *plugin;
};





PLUGIN_THREAD_HEADER(SoundLevelEffect, SoundLevelThread, SoundLevelWindow)




class SoundLevelEffect : public PluginAClient
{
public:
	SoundLevelEffect(PluginServer *server);
	~SoundLevelEffect();

	int is_realtime();
	void read_data(KeyFrame *keyframe);
	void save_data(KeyFrame *keyframe);
	int process_realtime(int64_t size, double *input_ptr, double *output_ptr);



	int load_defaults();
	int save_defaults();
	void reset();
	void update_gui();
	void render_gui(void *data, int size);



	PLUGIN_CLASS_MEMBERS(SoundLevelConfig, SoundLevelThread)

	double rms_accum;
	double max_accum;
	int accum_size;
};











#endif
