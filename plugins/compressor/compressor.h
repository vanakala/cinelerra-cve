#ifndef COMPRESSOR_H
#define COMPRESSOR_H



#include "defaults.inc"
#include "guicast.h"
#include "mutex.h"
#include "pluginaclient.h"
#include "vframe.inc"

class CompressorEffect;





class CompressorCanvas : public BC_SubWindow
{
public:
	CompressorCanvas(CompressorEffect *plugin, int x, int y, int w, int h);
	int button_press_event();
	int button_release_event();
	int cursor_motion_event();


	enum
	{
		NONE,
		DRAG
	};

	int current_point;
	int current_operation;
	CompressorEffect *plugin;
};


class CompressorPreview : public BC_TextBox
{
public:
	CompressorPreview(CompressorEffect *plugin, int x, int y);
	int handle_event();
	CompressorEffect *plugin;
};

class CompressorReaction : public BC_TextBox
{
public:
	CompressorReaction(CompressorEffect *plugin, int x, int y);
	int handle_event();
	CompressorEffect *plugin;
};

class CompressorClear : public BC_GenericButton
{
public:
	CompressorClear(CompressorEffect *plugin, int x, int y);
	int handle_event();
	CompressorEffect *plugin;
};

class CompressorX : public BC_TextBox
{
public:
	CompressorX(CompressorEffect *plugin, int x, int y);
	int handle_event();
	CompressorEffect *plugin;
};

class CompressorY : public BC_TextBox
{
public:
	CompressorY(CompressorEffect *plugin, int x, int y);
	int handle_event();
	CompressorEffect *plugin;
};

class CompressorTrigger : public BC_TextBox
{
public:
	CompressorTrigger(CompressorEffect *plugin, int x, int y);
	int handle_event();
	CompressorEffect *plugin;
};





class CompressorWindow : public BC_Window
{
public:
	CompressorWindow(CompressorEffect *plugin, int x, int y);
	void create_objects();
	void update();
	void update_textboxes();
	void update_canvas();
	int close_event();
	void draw_scales();
	
	
	CompressorCanvas *canvas;
	CompressorPreview *preview;
	CompressorReaction *reaction;
	CompressorClear *clear;
	CompressorX *x_text;
	CompressorY *y_text;
	CompressorTrigger *trigger;
	CompressorEffect *plugin;
};

PLUGIN_THREAD_HEADER(CompressorEffect, CompressorThread, CompressorWindow)


typedef struct
{
// DB from min_db - 0
	double x, y;
} compressor_point_t;

class CompressorConfig
{
public:
	CompressorConfig();

	void copy_from(CompressorConfig &that);
	int equivalent(CompressorConfig &that);
	void interpolate(CompressorConfig &prev, 
		CompressorConfig &next, 
		long prev_frame, 
		long next_frame, 
		long current_frame);

	int total_points();
	void remove_point(int number);
	void optimize();
// Return values of a specific point
	double get_y(int number);
	double get_x(int number);
// Returns db output given linear input
	double calculate_db(double x);
	int set_point(double x, double y);
	void dump();

	int trigger;
	double min_db;
	double preview_len;
	double reaction_len;
	double min_x, min_y;
	double max_x, max_y;
	ArrayList<compressor_point_t> levels;
};

class CompressorEffect : public PluginAClient
{
public:
	CompressorEffect(PluginServer *server);
	~CompressorEffect();

	int is_multichannel();
	int is_realtime();
	void read_data(KeyFrame *keyframe);
	void save_data(KeyFrame *keyframe);
	int process_realtime(long size, double **input_ptr, double **output_ptr);




	int load_defaults();
	int save_defaults();
	void reset();
	void update_gui();
	void delete_dsp();

	PLUGIN_CLASS_MEMBERS(CompressorConfig, CompressorThread)

	double **input_buffer;
	long input_size;
	long input_allocated;
	double *reaction_buffer;
	long reaction_allocated;
	long reaction_position;
	double current_coef;
	double previous_slope;
	double previous_intercept;
	double previous_max;
	double previous_coef;
	int max_counter;
	int total_samples;

// Same coefs are applied to all channels
	double *coefs;
	long coefs_allocated;
	long last_peak_age;
	double last_peak;
};


#endif
