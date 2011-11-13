
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */

#ifndef VFRAME_H
#define VFRAME_H

#include <stdint.h>
#include "arraylist.h"
#include "bchash.inc"
#include "bcpbuffer.inc"
#include "bctexture.inc"
#include "bcwindowbase.inc"
#include "datatype.h"
#include "colormodels.h"
#include "vframe.inc"

class PngReadFunction;


// Maximum number of prev or next effects to be pushed onto the stacks.
#define MAX_STACK_ELEMENTS 255



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
	VFrame();
	~VFrame();

	friend class PngReadFunction;

// Return 1 if the colormodel and dimensions are the same
// Used by FrameCache
	int equivalent(VFrame *src, int test_stacks = 0);

// Reallocate a frame without deleting the class
	void reallocate(unsigned char *data, 
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

	void set_compressed_memory(unsigned char *data,
		int data_size,
		int data_allocated);

// Read a PNG into the frame with alpha
	void read_png(unsigned char *data);

// if frame points to the same data as this return 1
	int equals(VFrame *frame);
// Test if frame already matches parameters
	int params_match(int w, int h, int color_model);

	void set_shm_offset(long offset);
	long get_shm_offset(void);

// direct copy with no alpha
	void copy_from(VFrame *frame);
// Required for YUV
	void clear_frame(void);
	void allocate_compressed_data(long bytes);

// Sequence number. -1 means invalid.  Passing frames to the encoder is
// asynchronous.  The sequence number must be preserved in the image itself
// to encode discontinuous frames.
	long get_number(void);
	void set_number(long number);

// Frame number in media file
	void set_frame_number(framenum number);
	framenum get_frame_number(void);
// Frame position and duration
	void set_source_pts(ptstime pts);
	ptstime get_source_pts(void);
	void set_pts(ptstime pts);
	ptstime get_pts(void);
	ptstime next_pts();
	int pts_in_frame(ptstime pts);
	void set_layer(int layer);
	int get_layer(void);
	void clear_pts(void);
	void copy_pts(VFrame *frame);
	void set_duration(ptstime duration);
	ptstime get_duration(void);

	long get_compressed_allocated();
	long get_compressed_size();
	void set_compressed_size(long size);
	int get_color_model();
// Get the data pointer
	unsigned char* get_data();
// return an array of pointers to rows
	unsigned char** get_rows();
// return yuv planes
	unsigned char* get_y(void);
	unsigned char* get_u(void);
	unsigned char* get_v(void);
	int get_w(void);
	int get_h(void);
	int get_w_fixed(void);
	int get_h_fixed(void);
	static void get_scale_tables(int *column_table, int *row_table, 
			int in_x1, int in_y1, int in_x2, int in_y2,
			int out_x1, int out_y1, int out_x2, int out_y2);
	int get_bytes_per_pixel(void);
	long get_bytes_per_line();
// Return 1 if the buffer is shared.
	int get_shared(void);



	static int calculate_bytes_per_pixel(int colormodel);
	static long calculate_data_size(int w, 
		int h, 
		int bytes_per_line = -1, 
		int color_model = BC_RGB888);
// Get size of uncompressed frame buffer
	long get_data_size();
	void rotate270(void);
	void rotate90(void);
	void flip_vert(void);

// Convenience storage.
// Returns -1 if not set.
	int get_field2_offset();
	int set_field2_offset(int value);
// Set keyframe status
	void set_keyframe(int value);
	int get_keyframe();
// Overlay src onto this with blending and translation of input.
// Source and this must have alpha
	void overlay(VFrame *src, 
		int out_x1, 
		int out_y1);

// If the opengl state is RAM, transfer image from RAM to the texture 
// referenced by this frame.
// If the opengl state is TEXTURE, do nothing.
// If the opengl state is SCREEN, switch the current drawable to the pbuffer and
// transfer the image to the texture with screen_to_texture.
// The opengl state is changed to TEXTURE.
// If no textures exist, textures are created.
// If the textures already exist, they are reused.
// Textures are resized to match the current dimensions.
// Must be called from a synchronous opengl thread after enable_opengl.
	void to_texture(void);

// Transfer from PBuffer to RAM.  Only used after Playback3D::overlay_sync
	void to_ram(void);

// Transfer contents of current pbuffer to texture, 
// creating a new texture if necessary.
// Coordinates are the coordinates in the drawable to copy.
	void screen_to_texture(int x = -1, 
		int y = -1, 
		int w = -1, 
		int h = -1);

// Transfer contents of texture to the current drawable.
// Just calls the vertex functions but doesn't initialize.  
// The coordinates are relative to the VFrame size and flipped to make
// the texture upright.
// The default coordinates are the size of the VFrame.
// flip_y flips the texture in the vertical direction and only used when
// writing to the final surface.
	void draw_texture(float in_x1, 
		float in_y1,
		float in_x2,
		float in_y2,
		float out_x1,
		float out_y1,
		float out_x2,
		float out_y2,
		int flip_y = 0);
// Draw the texture using the frame's size as the input and output coordinates.
	void draw_texture(int flip_y = 0);



// ================================ OpenGL functions ===========================
// Location of working image if OpenGL playback
	int get_opengl_state(void);
	void set_opengl_state(int value);
// OpenGL states
	enum
	{
// Undefined
		UNKNOWN,
// OpenGL image is in RAM
		RAM,
// OpenGL image is in texture
		TEXTURE,
// OpenGL image is composited in PBuffer or back buffer
		SCREEN
	};

// Texture ID
	int get_texture_id(void);
	void set_texture_id(int id);
// Get window ID the texture is bound to
	int get_window_id(void);
	int get_texture_w(void);
	int get_texture_h(void);
	int get_texture_components(void);


// Binds the opengl context to this frame's PBuffer
	void enable_opengl(void);

// Clears the pbuffer with the right values depending on YUV
	void clear_pbuffer(void);

// Get the pbuffer
	BC_PBuffer* get_pbuffer(void);

// Bind the frame's texture to GL_TEXTURE_2D and enable it.
// If a texture_unit is supplied, the texture unit is made active
// and the commands are run in the right sequence to 
// initialize it to our preferred specifications.
	void bind_texture(int texture_unit = -1);

// Create a frustum with 0,0 in the upper left and w,-h in the bottom right.
// Set preferred opengl settings.
	static void init_screen(int w, int h);
// Calls init_screen with the current frame's dimensions.
	void init_screen(void);

// Compiles and links the shaders into a program.
// Adds the program with put_shader.
// Returns the program handle.
// Requires a null terminated argument list of shaders to link together.
// At least one shader argument must have a main() function.  make_shader
// replaces all the main() functions with unique functions and calls them in
// sequence, so multiple independant shaders can be linked.
// x is a placeholder for va_arg and should be 0.
	static unsigned int make_shader(int x, ...);
	static void dump_shader(int shader_id);

// Because OpenGL is faster if multiple effects are combined, we need
// to provide ways for effects to aggregate.
// The prev_effect is the object providing the data to read_frame.
// The next_effect is the object which called read_frame.
// Push and pop are only called from Cinelerra internals, so
// if an object calls read_frame with a temporary, the stack before and after
// the temporary is lost.
	void push_prev_effect(const char *name);
	void pop_prev_effect(void);
	void push_next_effect(const char *name);
	void pop_next_effect(void);
// These are called by plugins to determine aggregation.
// They access any member of the stack based on the number argument.
// next effect 0 is the one that called read_frame most recently.
// prev effect 0 is the one that filled our call to read_frame.
	const char* get_next_effect(int number = 0);
	const char* get_prev_effect(int number = 0);

// It isn't enough to know the name of the neighboring effects.
// Relevant configuration parameters must be passed on.
	BC_Hash* get_params(void);

// Compare stacks and params from 2 images and return 1 if equal.
	int equal_stacks(VFrame *src);

// Copy stacks and params from another frame
// Replaces the stacks with the src stacks but only updates the params.
	void copy_stacks(VFrame *src);
// Updates the params with values from src
	void copy_params(VFrame *src);

// This clears the stacks and the param table
	void clear_stacks(void);

	void dump(int minmax = 0);
// Dump bitmamps to named file
	void dump_file(const char *filename);
	void dump_stacks(void);
	void dump_params(void);

private:

// Create a PBuffer matching this frame's dimensions and to be 
// referenced by this frame.  Does nothing if the pbuffer already exists.
// If the frame is resized, the PBuffer is deleted.
// Called by enable_opengl.
// This allows PBuffers, textures, and bitmaps to travel through the entire
// rendering chain without requiring the user to manage a lot of objects.
// Must be called from a synchronous opengl thread after enable_opengl.
	void create_pbuffer(void);



	void clear_objects(int do_opengl);
	void reset_parameters(int do_opengl);
	void create_row_pointers();
	void allocate_data(unsigned char *data, 
		long y_offset,
		long u_offset,
		long v_offset,
		int w, 
		int h, 
		int color_model, 
		long bytes_per_line);
	void calc_minmax8(unsigned char *buf, int len,
		unsigned int &avg, int &min, int &max);
	void calc_minmax8n(unsigned char *buf, int len, int pixlen,
		int *avg, int *min, int *max);
	void calc_minmax16(uint16_t *buf, int len, int pixlen,
		uint64_t *avg, uint64_t *min, uint64_t *max);
	void calc_minmaxfl(float *buf, int len, int pixlen,
		float *avg, float *min, float *max);

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

// OpenGL support
	int is_keyframe;
// State of the current texture
	BC_Texture *texture;
// State of the current PBuffer
	BC_PBuffer *pbuffer;

// Location of working image if OpenGL playback
	int opengl_state;

	ArrayList<char*> prev_effects;
	ArrayList<char*> next_effects;
	BC_Hash *params;
};


#endif
