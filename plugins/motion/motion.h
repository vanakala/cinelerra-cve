#ifndef MOTION_H
#define MOTION_H

#include <math.h>
#include <stdint.h>
#include <string.h>

#include "bcdisplayinfo.inc"
#include "defaults.h"
#include "filexml.inc"
#include "keyframe.inc"
#include "loadbalance.h"
#include "overlayframe.inc"
#include "pluginvclient.h"
#include "vframe.inc"

class MotionMain;
class MotionWindow;
class MotionEngine;



#define OVERSAMPLE 4

class MotionConfig
{
public:
	MotionConfig();

	int equivalent(MotionConfig &that);
	void copy_from(MotionConfig &that);
	void interpolate(MotionConfig &prev, 
		MotionConfig &next, 
		int64_t prev_frame, 
		int64_t next_frame, 
		int64_t current_frame);

	int block_size;
	int search_radius;
	int magnitude;
	int return_speed;
	int draw_vectors;
	int mode;
	enum
	{
		TRACK,
		STABILIZE,
		DRAW_VECTORS
	};
};


class MotionSearchRadius : public BC_IPot
{
public:
	MotionSearchRadius(MotionMain *plugin, 
		int x, 
		int y);
	int handle_event();
	MotionMain *plugin;
};

class MotionBlockSize : public BC_IPot
{
public:
	MotionBlockSize(MotionMain *plugin, 
		int x, 
		int y);
	int handle_event();
	MotionMain *plugin;
};

class MotionMagnitude : public BC_IPot
{
public:
	MotionMagnitude(MotionMain *plugin, 
		int x, 
		int y);
	int handle_event();
	MotionMain *plugin;
};

class MotionReturnSpeed : public BC_IPot
{
public:
	MotionReturnSpeed(MotionMain *plugin, 
		int x, 
		int y);
	int handle_event();
	MotionMain *plugin;
};

class MotionStabilize : public BC_Radial
{
public:
	MotionStabilize(MotionMain *plugin, 
		MotionWindow *gui,
		int x, 
		int y);
	int handle_event();
	MotionWindow *gui;
	MotionMain *plugin;
};

class MotionTrack : public BC_Radial
{
public:
	MotionTrack(MotionMain *plugin, 
		MotionWindow *gui,
		int x, 
		int y);
	int handle_event();
	MotionWindow *gui;
	MotionMain *plugin;
};

class MotionDrawVectors : public BC_Radial
{
public:
	MotionDrawVectors(MotionMain *plugin, 
		MotionWindow *gui,
		int x, 
		int y);
	int handle_event();
	MotionMain *plugin;
	MotionWindow *gui;
};

class MotionWindow : public BC_Window
{
public:
	MotionWindow(MotionMain *plugin, int x, int y);
	~MotionWindow();

	int create_objects();
	int close_event();
	void update_mode();

	MotionSearchRadius *radius;
	MotionBlockSize *block_size;
	MotionMagnitude *magnitude;
	MotionReturnSpeed *return_speed;
	MotionStabilize *stabilize;
	MotionTrack *track;
	MotionDrawVectors *vectors;
	MotionMain *plugin;
};



PLUGIN_THREAD_HEADER(MotionMain, MotionThread, MotionWindow)


typedef struct
{
	int x;
	int y;
	int dx_oversampled;
	int dy_oversampled;
	int valid;
} macroblock_t;


class MotionMain : public PluginVClient
{
public:
	MotionMain(PluginServer *server);
	~MotionMain();

	int process_realtime(VFrame **input_ptr, VFrame **output_ptr);
	int is_multichannel();
	int is_realtime();
	int load_defaults();
	int save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void update_gui();

	PLUGIN_CLASS_MEMBERS(MotionConfig, MotionThread)

	static void oversample(int32_t *dst, 
		VFrame *src,
		int x1,
		int x2,
		int y1,
		int y2,
		int dst_stride);

	static void draw_pixel(VFrame *frame, int x, int y);
	static void draw_line(VFrame *frame, int x1, int y1, int x2, int y2);

	VFrame *prev_frame;
	VFrame *current_frame;
	VFrame *temp_frame;
	MotionEngine *engine;
	OverlayFrame *overlayer;
	macroblock_t *macroblocks;
	int total_macroblocks;

// Absolute position of current frame relative to center * OVERSAMPLE
	int current_dx;
	int current_dy;
// Oversampled current frame for motion estimation
	int32_t *search_area;
	int search_size;
};


class MotionPackage : public LoadPackage
{
public:
	MotionPackage();
	macroblock_t *macroblock;
};

class MotionUnit : public LoadClient
{
public:
	MotionUnit(MotionEngine *server, MotionMain *plugin);
	~MotionUnit();

	void process_package(LoadPackage *package);
	int64_t abs_diff(int32_t *search_pixel,
		int32_t *block_pixel,
		int search_side,
		int block_side);

	MotionEngine *server;
	MotionMain *plugin;
// Oversampled macroblock in previous frame
	int32_t *block_area;
	int block_size;
// Results of scan go here so the same places don't need to be scanned twice.
	int64_t *scan_result;
	int scan_result_size;
};

class MotionEngine : public LoadServer
{
public:
	MotionEngine(MotionMain *plugin, 
		int total_clients, 
		int total_packages);
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
	MotionMain *plugin;
};






#endif






