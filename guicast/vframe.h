#ifndef VFRAME_H
#define VFRAME_H

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
		long bytes_per_line = -1);
	VFrame(unsigned char *data, 
		long y_offset,
		long u_offset,
		long v_offset,
		int w, 
		int h, 
		int color_model = BC_RGBA8888, 
		long bytes_per_line = -1);
// Create a frame with the png image
	VFrame(unsigned char *png_data);
	VFrame(VFrame &vframe);
// Create new frame for compressed data.
// Can't share data because the data is dynamically allocated.
	VFrame();
	~VFrame();

	friend class PngReadFunction;

// Reallocate a frame without deleting the class
	int reallocate(unsigned char *data, 
		long y_offset,
		long u_offset,
		long v_offset,
		int w, 
		int h, 
		int color_model, 
		long bytes_per_line);

	void set_memory(unsigned char *data, 
		long y_offset,
		long u_offset,
		long v_offset);

// Read a PNG into the frame with alpha
	int read_png(unsigned char *data);

// if frame points to the same data as this return 1
	int equals(VFrame *frame);
// Test if frame already matches parameters
	int params_match(int w, int h, int color_model);

	long set_shm_offset(long offset);
	long get_shm_offset();

// direct copy with no alpha
	int copy_from(VFrame *frame);
// Direct copy with alpha from 0 to 1.
	int replace_from(VFrame *frame, float alpha);

	int apply_fade(float alpha);
// Required for YUV
	int clear_frame();
	int allocate_compressed_data(long bytes);

// Sequence number. -1 means invalid.  Passing frames to the encoder is
// asynchronous.  The sequence number must be preserved in the image itself
// to encode discontinuous frames.
	long get_number();
	void set_number(long number);

	long get_compressed_allocated();
	long get_compressed_size();
	long set_compressed_size(long size);
	int get_color_model();
// Get the data pointer
	unsigned char* get_data();
// return an array of pointers to rows
	unsigned char** get_rows();       
// return yuv planes
	unsigned char* get_y(); 
	unsigned char* get_u(); 
	unsigned char* get_v(); 
	int get_w();
	int get_h();
	int get_w_fixed();
	int get_h_fixed();
	static int get_scale_tables(int *column_table, int *row_table, 
			int in_x1, int in_y1, int in_x2, int in_y2,
			int out_x1, int out_y1, int out_x2, int out_y2);
	int get_bytes_per_pixel();
	long get_bytes_per_line();
	static int calculate_bytes_per_pixel(int colormodel);
	static long calculate_data_size(int w, 
		int h, 
		int bytes_per_line = -1, 
		int color_model = BC_RGB888);
	long get_data_size();
	void rotate270();
	void rotate90();
	void flip_vert();

// Convenience storage.
// Returns -1 if not set.
	int get_field2_offset();
	int set_field2_offset(int value);
// Overlay src onto this with blending and translation of input.
// Source and this must have alpha
	void overlay(VFrame *src, 
		int out_x1, 
		int out_y1);

private:
	int clear_objects();
	int reset_parameters();
	void create_row_pointers();
	int allocate_data(unsigned char *data, 
		long y_offset,
		long u_offset,
		long v_offset,
		int w, 
		int h, 
		int color_model, 
		long bytes_per_line);

// Convenience storage
	int field2_offset;
// Data is pointing to someone else's buffer.
	int shared; 
	long shm_offset;
// If not set by user, is calculated from color_model
	long bytes_per_line;
	int bytes_per_pixel;
// Image data
	unsigned char *data;
// Pointers to the start of each row
	unsigned char **rows;
// One of the #defines
	int color_model;
// Allocated space for compressed data
	long compressed_allocated;
// Size of stored compressed image
	long compressed_size;   
// Pointers to yuv planes
	unsigned char *y, *u, *v;
	long y_offset;
	long u_offset;
	long v_offset;
// Dimensions of frame
	int w, h;
// Info for reading png images
	unsigned char *image;
	long image_offset;
	long image_size;
// For writing discontinuous frames in background rendering
	long sequence_number;
};


#endif
