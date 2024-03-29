// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef MOTION_H
#define MOTION_H

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME
#define PLUGIN_USES_TMPFRAME
#define PLUGIN_IS_MULTICHANNEL
#define PLUGIN_MAX_CHANNELS 2

#define PLUGIN_TITLE N_("Motion")
#define PLUGIN_CLASS MotionMain
#define PLUGIN_CONFIG_CLASS MotionConfig
#define PLUGIN_THREAD_CLASS MotionThread
#define PLUGIN_GUI_CLASS MotionWindow

#include "pluginmacros.h"

#include <math.h>
#include <stdint.h>
#include <string.h>

#include "affine.inc"
#include "bchash.inc"
#include "filexml.inc"
#include "guidelines.inc"
#include "keyframe.inc"
#include "loadbalance.h"
#include "overlayframe.inc"
#include "pluginvclient.h"
#include "rotateframe.inc"
#include "vframe.inc"

class MotionScan;
class RotateScan;

#define OVERSAMPLE 4

// Limits of global range in percent
#define MIN_RADIUS 1
#define MAX_RADIUS 50

// Limits of rotation range in degrees
#define MIN_ROTATION 1
#define MAX_ROTATION 25

// Limits of block size in percent.
#define MIN_BLOCK 1
#define MAX_BLOCK 100

// Limits of block count
#define MIN_BLOCKS 1
#define MAX_BLOCKS 200

// Precision of rotation
#define MIN_ANGLE 0.0001

#define MOTION_FILE "/tmp/motion"
#define ROTATION_FILE "/tmp/rotate"

class MotionConfig
{
public:
	MotionConfig();

	int equivalent(MotionConfig &that);
	void copy_from(MotionConfig &that);
	void interpolate(MotionConfig &prev, 
		MotionConfig &next, 
		ptstime prev_pts,
		ptstime next_pts,
		ptstime current_pts);
	void boundaries();

	int block_count;
	int global_range_w;
	int global_range_h;
	int rotation_range;
	int magnitude;
	int return_speed;
	int draw_vectors;
// Percent of image size
	int global_block_w;
	int global_block_h;
	int rotation_block_w;
	int rotation_block_h;
// Number of search positions in each refinement of the log search
	int global_positions;
	int rotate_positions;
// Block position in percentage 0 - 100
	double block_x;
	double block_y;

	int horizontal_only;
	int vertical_only;
	int global;
	int rotate;
	int addtrackedframeoffset;
// Track or stabilize, single pixel, scan only, or nothing
	int mode1;
// Recalculate, no calculate, save, or load coordinates from disk
	int mode2;
// Track a single frame, previous frame, or previous frame same block
	int mode3;
	enum
	{
// mode1
		TRACK,
		STABILIZE,
		TRACK_PIXEL,
		STABILIZE_PIXEL,
		NOTHING,
// mode2
		RECALCULATE,
		SAVE,
		LOAD,
		NO_CALCULATE,
// mode3
		TRACK_SINGLE,
		TRACK_PREVIOUS,
		PREVIOUS_SAME_BLOCK
	};

// Pts of single frame to track relative to timeline start
	ptstime track_pts;
// Stablisation gains
	double stab_gain_x;
	double stab_gain_y;
	PLUGIN_CONFIG_CLASS_MEMBERS
};


class MotionMain : public PluginVClient
{
public:
	MotionMain(PluginServer *server);
	~MotionMain();

	void process_tmpframes(VFrame **frame);
	void reset_plugin();
	void process_global();
	void process_rotation();
	void draw_vectors();
	void load_defaults();
	void save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);

	PLUGIN_CLASS_MEMBERS

	int abs_diff(unsigned char *prev_ptr,
		unsigned char *current_ptr,
		int row_bytes,
		int w,
		int h,
		int color_model);
	int abs_diff_sub(unsigned char *prev_ptr,
		unsigned char *current_ptr,
		int row_bytes,
		int w,
		int h,
		int color_model,
		int sub_x,
		int sub_y);

	static void clamp_scan(int w, 
		int h, 
		int *block_x1,
		int *block_y1,
		int *block_x2,
		int *block_y2,
		int *scan_x1,
		int *scan_y1,
		int *scan_x2,
		int *scan_y2,
		int use_absolute);

	void draw_arrow(GuideFrame *gf, int x1, int y1, int x2, int y2);

	MotionScan *engine;
	RotateScan *motion_rotate;
	OverlayFrame *overlayer;
	AffineEngine *rotate_engine;

// Accumulation of all global tracks since the plugin start.
// Multiplied by OVERSAMPLE.
	int total_dx;
	int total_dy;

// Rotation motion tracking
	double total_angle;

// Current motion vector for drawing vectors
	int current_dx;
	int current_dy;
	double current_angle;

// The layer to track motion in.
	int reference_layer;

// The previous reference frame
	VFrame *prev_ref;
// The current reference frame
	VFrame *current_ref;
// The input target frame
	VFrame *target_src;
// The output target frame
	VFrame *target_dst;

	int width;
	int height;
};


class MotionScanPackage : public LoadPackage
{
public:
	MotionScanPackage();

// For multiple blocks
	int block_x1, block_y1, block_x2, block_y2;
	int scan_x1, scan_y1, scan_x2, scan_y2;
	int dx;
	int dy;
	int valid;
// For single block
	int pixel;
	int difference1;
	int difference2;
};


class MotionScanCache
{
public:
	MotionScanCache(int x, int y, int difference);

	int x, y;
	int difference;
};


class MotionScanUnit : public LoadClient
{
public:
	MotionScanUnit(MotionScan *server, MotionMain *plugin);
	~MotionScanUnit();

	void process_package(LoadPackage *package);
	int get_cache(int x, int y);
	void put_cache(int x, int y, int difference);

	MotionScan *server;
	MotionMain *plugin;

	ArrayList<MotionScanCache*> cache;
	Mutex *cache_lock;
};


class MotionScan : public LoadServer
{
public:
	MotionScan(MotionMain *plugin, 
		int total_clients, 
		int total_packages);
	~MotionScan();

	friend class MotionScanUnit;

	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();

// Invoke the motion engine for a search
// Frame before motion
	void scan_frame(VFrame *previous_frame,
// Frame after motion
		VFrame *current_frame);
	int get_cache(int x, int y);
	void put_cache(int x, int y, int difference);

// Change between previous frame and current frame multiplied by 
// OVERSAMPLE
	int dx_result;
	int dy_result;

private:
	VFrame *previous_frame;
// Frame after motion
	VFrame *current_frame;
	MotionMain *plugin;
	int skip;
// For single block
	int block_x1;
	int block_x2;
	int block_y1;
	int block_y2;
	int scan_x1;
	int scan_y1;
	int scan_x2;
	int scan_y2;
	int total_pixels;
	int total_steps;
	int subpixel;

	ArrayList<MotionScanCache*> cache;
	Mutex *cache_lock;
};


class RotateScanPackage : public LoadPackage
{
public:
	RotateScanPackage();

	double angle;
	int difference;
};


class RotateScanCache
{
public:
	RotateScanCache(double angle, int difference);

	double angle;
	int difference;
};


class RotateScanUnit : public LoadClient
{
public:
	RotateScanUnit(RotateScan *server, MotionMain *plugin);
	~RotateScanUnit();

	void process_package(LoadPackage *package);

	RotateScan *server;
	MotionMain *plugin;
	AffineEngine *rotater;
};


class RotateScan : public LoadServer
{
public:
	RotateScan(MotionMain *plugin, 
		int total_clients, 
		int total_packages);
	~RotateScan();

	friend class RotateScanUnit;

	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();

// Invoke the motion engine for a search
// Frame before rotation
	double scan_frame(VFrame *previous_frame,
// Frame after rotation
		VFrame *current_frame,
// Pivot
		int block_x,
		int block_y);
	int get_cache(double angle);
	void put_cache(double angle, int difference);

// Angle result
	double result;

private:
	VFrame *previous_frame;
// Frame after motion
	VFrame *current_frame;

	MotionMain *plugin;
	int skip;

// Pivot
	int block_x;
	int block_y;
// Block to rotate
	int block_x1;
	int block_x2;
	int block_y1;
	int block_y2;
// Area to compare
	int scan_x;
	int scan_y;
	int scan_w;
	int scan_h;
// Range of angles to compare
	double scan_angle1, scan_angle2;
	int total_steps;

	ArrayList<RotateScanCache*> cache;
	Mutex *cache_lock;
};

#endif
