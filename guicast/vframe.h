// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef VFRAME_H
#define VFRAME_H

#include <stdint.h>
#include "arraylist.h"
#include "bchash.inc"
#include "bcwindowbase.inc"
#include "datatype.h"
#include "colormodels.h"
#include "vframe.inc"

class PngReadFunction;

class VFrame
{
public:
// Create new frame with shared data if *data is nonzero.
// Pass 0 to *data if private data is desired.
	VFrame(unsigned char *data, 
		int w, 
		int h, 
		int color_model = BC_RGBA8888, 
		int bytes_per_line = -1);
	VFrame(unsigned char *data, 
		int y_offset,
		int u_offset,
		int v_offset,
		int w, 
		int h, 
		int color_model = BC_RGBA8888, 
		int bytes_per_line = -1);
// Create a frame with the png image
	VFrame(unsigned char *png_data);
	VFrame(VFrame &vframe);
// Create new frame for compressed data.
	VFrame();
	~VFrame();

	friend class PngReadFunction;

// Return 1 if the colormodel and dimensions are the same
// Used by FrameCache
	int equivalent(VFrame *src);

// Reallocate a frame without deleting the class
	void reallocate(unsigned char *data, 
		int y_offset,
		int u_offset,
		int v_offset,
		int w, 
		int h, 
		int color_model, 
		int bytes_per_line);

	void set_memory(unsigned char *data, 
		int y_offset,
		int u_offset,
		int v_offset);

	void set_compressed_memory(unsigned char *data,
		int data_size,
		int data_allocated);

// Read a PNG into the frame with alpha
	void read_png(unsigned char *data);

// if frame points to the same data as this return 1
	int equals(VFrame *frame);
// Test if frame already matches parameters
	int params_match(int w, int h, int color_model);

// copy with color conversion and rescale
	void transfer_from(VFrame *frame);
// direct copy with no alpha
	void copy_from(VFrame *frame, int do_copy_pts = 1);
// Required for YUV
	void clear_frame(void);
	void allocate_compressed_data(int bytes);

// Frame number in media file
	void set_frame_number(framenum number);
	framenum get_frame_number(void);
// Frame position and duration
	void set_source_pts(ptstime pts);
	ptstime get_source_pts(void);
	void set_pts(ptstime pts);
	ptstime get_pts(void);
	ptstime next_source_pts();
	ptstime next_pts();
	int get_status();
	void clear_status();
	void set_transparent();
	void clear_transparent();
	int is_transparent();
	void merge_status(VFrame *that);
	void merge_color(int color);
	int pts_in_frame_source(ptstime pts, ptstime accuracy = FRAME_ACCURACY);
	int pts_in_frame(ptstime pts, ptstime accuracy = FRAME_ACCURACY);
	void set_layer(int layer);
	int get_layer(void);
	void clear_pts(void);
	void copy_pts(VFrame *frame);
	void set_duration(ptstime duration);
	ptstime get_duration(void);

	int get_compressed_allocated();
	int get_compressed_size();
	void set_compressed_size(int size);
	int get_color_model();
// Get the data pointer
	unsigned char* get_data();
// return pointer to the row
	unsigned char* get_row_ptr(int num);
// return an array of pointers to rows
	unsigned char** get_rows();
	void delete_row_ptrs();
// return yuv planes
	unsigned char* get_y(void);
	unsigned char* get_u(void);
	unsigned char* get_v(void);
	int get_w(void);
	int get_h(void);
	int get_bytes_per_pixel(void);
	int get_bytes_per_line();
	int get_pixels_per_line();
// Return 1 if the buffer is shared.
	int get_shared(void);
	double get_pixel_aspect();
	void set_pixel_aspect(double aspect);

	static size_t calculate_data_size(int w, 
		int h, 
		int bytes_per_line = -1, 
		int color_model = BC_RGB888);
// Get size of uncompressed frame buffer
	size_t get_data_size();
	void flip_vert(void);

// Debugging functions
	void dump(int indent = 0, int minmax = 0);
// Dump bitmamps to named file
	void dump_file(const char *filename);

	static void calc_minmax8(unsigned char *buf, int len,
		unsigned int &avg, int &min, int &max);
	static void calc_minmax8n(unsigned char *buf, int pixlen,
		int width, int height, int bytes_per_line,
		int *avg, int *min, int *max);
	static void calc_minmax16(uint16_t *buf, int pixlen,
		int width, int height, int bytes_per_line,
		uint64_t *avg, uint64_t *min, uint64_t *max);
	static void calc_minmaxfl(float *buf, int pixlen,
		int width, int height, int bytes_per_line,
		float *avg, float *min, float *max);

private:
	void clear_objects();
	void reset_parameters();
	void create_row_pointers();
	void allocate_data(unsigned char *data, 
		int y_offset,
		int u_offset,
		int v_offset,
		int w, 
		int h, 
		int color_model, 
		int bytes_per_line);

// Frame position in source
	ptstime source_pts;
// Frame position in project
	ptstime pts;
// Frame duration
	ptstime duration;
// Frame number in source
	framenum frame_number;
// Layer
	int layer;
// Pixel aspect ratio of frame (0 - not set)
	double pixel_aspect;
// Status of the frame: bits from vframe.inc
	int status;

// Data is pointing to someone else's buffer.
	int shared;
// If not set by user, is calculated from color_model
	int bytes_per_line;
	int bytes_per_pixel;
// Image data
	unsigned char *data;
// Pointers to the start of each row
	unsigned char **rows;
// One of the #defines
	int color_model;
// Allocated space for compressed data
	int compressed_allocated;
// Size of stored compressed image
	int compressed_size;
// Pointers to yuv planes
	unsigned char *y, *u, *v;
	int y_offset;
	int u_offset;
	int v_offset;
// Dimensions of frame
	int w, h;
// Info for reading png images
	unsigned char *image;
	int image_offset;
	int image_size;
};

#endif
