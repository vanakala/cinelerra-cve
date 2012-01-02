
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

#include <png.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "bchash.h"
#include "bcpbuffer.h"
#include "bcsignals.h"
#include "bcsynchronous.h"
#include "bctexture.h"
#include "bcwindowbase.h"
#include "clip.h"
#include "colormodels.h"
#include "vframe.h"

class PngReadFunction
{
public:
	static void png_read_function(png_structp png_ptr,
		png_bytep data,
		png_size_t length)
	{
		VFrame *frame = (VFrame*)png_get_io_ptr(png_ptr);
		if(frame->image_size - frame->image_offset < length) 
			length = frame->image_size - frame->image_offset;

		memcpy(data, &frame->image[frame->image_offset], length);
		frame->image_offset += length;
	};
};


VFrame::VFrame(unsigned char *png_data)
{
	reset_parameters(1);
	params = new BC_Hash;
	read_png(png_data);
}

VFrame::VFrame(VFrame &frame)
{
	reset_parameters(1);
	params = new BC_Hash;
	allocate_data(0, 0, 0, 0, frame.w, frame.h, frame.color_model, frame.bytes_per_line);
	memcpy(data, frame.data, bytes_per_line * h);
	copy_stacks(&frame);
	copy_pts(&frame);
}

VFrame::VFrame(unsigned char *data, 
	int w, 
	int h, 
	int color_model, 
	long bytes_per_line)
{
	reset_parameters(1);
	params = new BC_Hash;
	allocate_data(data, 0, 0, 0, w, h, color_model, bytes_per_line);
}

VFrame::VFrame(unsigned char *data, 
		long y_offset,
		long u_offset,
		long v_offset, 
		int w, 
		int h, 
		int color_model, 
		long bytes_per_line)
{
	reset_parameters(1);
	params = new BC_Hash;
	allocate_data(data, 
		y_offset, 
		u_offset, 
		v_offset, 
		w, 
		h, 
		color_model, 
		bytes_per_line);
}

VFrame::VFrame()
{
	reset_parameters(1);
	params = new BC_Hash;
	this->color_model = BC_COMPRESSED;
}

VFrame::~VFrame()
{
	clear_objects(1);
// Delete effect stack
	prev_effects.remove_all_objects();
	next_effects.remove_all_objects();
	delete params;
}

int VFrame::equivalent(VFrame *src, int test_stacks)
{
	return (src->get_color_model() == get_color_model() &&
		src->get_w() == get_w() &&
		src->get_h() == get_h() &&
		src->bytes_per_line == bytes_per_line &&
		(!test_stacks || equal_stacks(src)));
}

void VFrame::set_shm_offset(long offset)
{
	shm_offset = offset;
}

long VFrame::get_shm_offset(void)
{
	return shm_offset;
}

int VFrame::get_shared(void)
{
	return shared;
}

int VFrame::params_match(int w, int h, int color_model)
{
	return (this->w == w &&
		this->h == h &&
		this->color_model == color_model);
}


void VFrame::reset_parameters(int do_opengl)
{
	field2_offset = -1;
	shared = 0;
	shm_offset = 0;
	bytes_per_line = 0;
	data = 0;
	rows = 0;
	color_model = 0;
	compressed_allocated = 0;
	compressed_size = 0;   // Size of current image
	w = 0;
	h = 0;
	y = u = v = 0;
	y_offset = 0;
	u_offset = 0;
	v_offset = 0;
	sequence_number = -1;
	clear_pts();
	is_keyframe = 0;

	if(do_opengl)
	{
// By default, anything is going to be done in RAM
		opengl_state = VFrame::RAM;
		pbuffer = 0;
		texture = 0;
	}

	prev_effects.set_array_delete();
	next_effects.set_array_delete();
}

void VFrame::clear_objects(int do_opengl)
{
// Remove texture
	if(do_opengl)
	{
		delete texture;
		texture = 0;

		delete pbuffer;
		pbuffer = 0;
	}

// Delete data
	if(!shared)
	{
		if(data) delete [] data;
		data = 0;
	}

// Delete row pointers
	switch(color_model)
	{
	case BC_COMPRESSED:
	case BC_YUV420P:
		break;

	default:
		delete [] rows;
		break;
	}
}

int VFrame::get_field2_offset()
{
	return field2_offset;
}

int VFrame::set_field2_offset(int value)
{
	this->field2_offset = value;
	return 0;
}

void VFrame::set_keyframe(int value)
{
	this->is_keyframe = value;
}

int VFrame::get_keyframe()
{
	return is_keyframe;
}


int VFrame::calculate_bytes_per_pixel(int color_model)
{
	return cmodel_calculate_pixelsize(color_model);
}

long VFrame::get_bytes_per_line()
{
	return bytes_per_line;
}

long VFrame::get_data_size()
{
	return calculate_data_size(w, h, bytes_per_line, color_model) - 4;
}

long VFrame::calculate_data_size(int w, int h, int bytes_per_line, int color_model)
{
	return cmodel_calculate_datasize(w, h, bytes_per_line, color_model);
}

void VFrame::create_row_pointers()
{
	switch(color_model)
	{
	case BC_YUV420P:
	case BC_YUV411P:
		if(!this->v_offset)
		{
			this->y_offset = 0;
			this->u_offset = w * h;
			this->v_offset = w * h + w * h / 4;
		}
		y = this->data + this->y_offset;
		u = this->data + this->u_offset;
		v = this->data + this->v_offset;
		break;

	case BC_YUV422P:
		if(!this->v_offset)
		{
			this->y_offset = 0;
			this->u_offset = w * h;
			this->v_offset = w * h + w * h / 2;
		}
		y = this->data + this->y_offset;
		u = this->data + this->u_offset;
		v = this->data + this->v_offset;
		break;

	default:
		rows = new unsigned char*[h];
		for(int i = 0; i < h; i++)
		{
			rows[i] = &this->data[i * this->bytes_per_line];
		}
		break;
	}
}

void VFrame::allocate_data(unsigned char *data, 
	long y_offset,
	long u_offset,
	long v_offset,
	int w, 
	int h, 
	int color_model, 
	long bytes_per_line)
{
	this->w = w;
	this->h = h;
	this->color_model = color_model;
	this->bytes_per_pixel = calculate_bytes_per_pixel(color_model);
	this->y_offset = this->u_offset = this->v_offset = 0;

	if(bytes_per_line >= 0)
	{
		this->bytes_per_line = bytes_per_line;
	}
	else
		this->bytes_per_line = this->bytes_per_pixel * w;

// Allocate data + padding for MMX
	if(data)
	{
		shared = 1;
		this->data = data;
		this->y_offset = y_offset;
		this->u_offset = u_offset;
		this->v_offset = v_offset;
	}
	else
	{
		shared = 0;
		int size = calculate_data_size(this->w, 
			this->h, 
			this->bytes_per_line, 
			this->color_model);
		this->data = new unsigned char[size];
	}

// Create row pointers
	create_row_pointers();
}

void VFrame::set_memory(unsigned char *data, 
		long y_offset,
		long u_offset,
		long v_offset)
{
	shared = 1;
	this->data = data;
	this->y_offset = y_offset;
	this->u_offset = u_offset;
	this->v_offset = v_offset;
	y = this->data + this->y_offset;
	u = this->data + this->u_offset;
	v = this->data + this->v_offset;
	create_row_pointers();
}

void VFrame::set_compressed_memory(unsigned char *data,
	int data_size,
	int data_allocated)
{
	clear_objects(0);
	shared = 1;
	this->data = data;
	this->compressed_allocated = data_allocated;
	this->compressed_size = data_size;
}


// Reallocate uncompressed buffer with or without alpha
void VFrame::reallocate(unsigned char *data, 
		long y_offset,
		long u_offset,
		long v_offset,
		int w, 
		int h, 
		int color_model, 
		long bytes_per_line)
{
	clear_objects(0);
	reset_parameters(0);
	allocate_data(data, 
		y_offset, 
		u_offset, 
		v_offset, 
		w, 
		h, 
		color_model, 
		bytes_per_line);
}

void VFrame::allocate_compressed_data(long bytes)
{
	if(bytes < 1) return;
// Want to preserve original contents
	if(data && compressed_allocated < bytes)
	{
		unsigned char *new_data = new unsigned char[bytes];
		memcpy(new_data, data, compressed_allocated);
		delete [] data;
		data = new_data;
		compressed_allocated = bytes;
	}
	else
	if(!data)
	{
		data = new unsigned char[bytes];
		compressed_allocated = bytes;
		compressed_size = 0;
	}
}

void VFrame::read_png(unsigned char *data)
{
	png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
	png_infop info_ptr = png_create_info_struct(png_ptr);
	int new_color_model;
	int have_alpha = 0;

	image_offset = 0;
	image = data + 4;
	image_size = (((unsigned long)data[0]) << 24) | 
		(((unsigned long)data[1]) << 16) | 
		(((unsigned long)data[2]) << 8) | 
		(unsigned char)data[3];
	png_set_read_fn(png_ptr, this, PngReadFunction::png_read_function);
	png_read_info(png_ptr, info_ptr);

	w = png_get_image_width(png_ptr, info_ptr);
	h = png_get_image_height(png_ptr, info_ptr);

	int src_color_model = png_get_color_type(png_ptr, info_ptr);

	/* tell libpng to strip 16 bit/color files down to 8 bits/color */
	png_set_strip_16(png_ptr);

	/* extract multiple pixels with bit depths of 1, 2, and 4 from a single
	 * byte into separate bytes (useful for paletted and grayscale images).
	 */
	png_set_packing(png_ptr);

	/* expand paletted colors into true RGB triplets */
	if (src_color_model == PNG_COLOR_TYPE_PALETTE)
		png_set_expand(png_ptr);

	/* expand grayscale images to the full 8 bits from 1, 2, or 4 bits/pixel */
	if (src_color_model == PNG_COLOR_TYPE_GRAY && png_get_bit_depth(png_ptr, info_ptr) < 8)
		png_set_expand(png_ptr);

	if (src_color_model == PNG_COLOR_TYPE_GRAY ||
			src_color_model == PNG_COLOR_TYPE_GRAY_ALPHA)
		png_set_gray_to_rgb(png_ptr);

	/* expand paletted or RGB images with transparency to full alpha channels
	 * so the data will be available as RGBA quartets */
	if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
	{
		have_alpha = 1;
		png_set_expand(png_ptr);
	}

	switch(src_color_model)
	{
		case PNG_COLOR_TYPE_GRAY:
		case PNG_COLOR_TYPE_RGB:
			new_color_model = BC_RGB888;
			break;

		case PNG_COLOR_TYPE_PALETTE:
			if(have_alpha)
				new_color_model = BC_RGBA8888;
			else
				new_color_model = BC_RGB888;
			break;

		case PNG_COLOR_TYPE_GRAY_ALPHA:
		case PNG_COLOR_TYPE_RGB_ALPHA:
		default:
			new_color_model = BC_RGBA8888;
			break;
	}

	reallocate(NULL, 
		0, 
		0, 
		0, 
		w, 
		h, 
		new_color_model,
		-1);

	png_read_image(png_ptr, get_rows());

	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
}

unsigned char* VFrame::get_data()
{
	return data;
}

long VFrame::get_compressed_allocated()
{
	return compressed_allocated;
}

long VFrame::get_compressed_size()
{
	return compressed_size;
}

void VFrame::set_compressed_size(long size)
{
	compressed_size = size;
}

int VFrame::get_color_model()
{
	return color_model;
}


int VFrame::equals(VFrame *frame)
{
	if(frame->data == data) 
		return 1;
	else
		return 0;
}

#define ZERO_YUV(components, type, max) \
{ \
	for(int i = 0; i < h; i++) \
	{ \
		type *row = (type*)get_rows()[i]; \
		for(int j = 0; j < w; j++) \
		{ \
			row[j * components] = 0; \
			row[j * components + 1] = (max + 1) / 2; \
			row[j * components + 2] = (max + 1) / 2; \
			if(components == 4) row[j * components + 3] = 0; \
		} \
	} \
}

void VFrame::clear_frame(void)
{
	switch(color_model)
	{
	case BC_COMPRESSED:
		break;

	case BC_YUV420P:
		memset(data, 0, h * w * 2);
		break;

	case BC_YUV888:
		ZERO_YUV(3, unsigned char, 0xff);
		break;

	case BC_YUVA8888:
		ZERO_YUV(4, unsigned char, 0xff);
		break;

	case BC_YUV161616:
		ZERO_YUV(3, uint16_t, 0xffff);
		break;

	case BC_YUVA16161616:
		ZERO_YUV(4, uint16_t, 0xffff);
		break;

	default:
		memset(data, 0, h * bytes_per_line);
		break;
	}
}

void VFrame::rotate90(void)
{
// Allocate new frame
	int new_w = h, new_h = w, new_bytes_per_line = bytes_per_pixel * new_w;
	unsigned char *new_data = new unsigned char[calculate_data_size(new_w, new_h, new_bytes_per_line, color_model)];
	unsigned char **new_rows = new unsigned char*[new_h];
	for(int i = 0; i < new_h; i++)
		new_rows[i] = &new_data[new_bytes_per_line * i];

// Copy data
	for(int in_y = 0, out_x = new_w - 1; in_y < h; in_y++, out_x--)
	{
		for(int in_x = 0, out_y = 0; in_x < w; in_x++, out_y++)
		{
			for(int k = 0; k < bytes_per_pixel; k++)
			{
				new_rows[out_y][out_x * bytes_per_pixel + k] = 
					rows[in_y][in_x * bytes_per_pixel + k];
			}
		}
	}

// Swap frames
	clear_objects(0);
	data = new_data;
	rows = new_rows;
	bytes_per_line = new_bytes_per_line;
	w = new_w;
	h = new_h;
}

void VFrame::rotate270(void)
{
// Allocate new frame
	int new_w = h, new_h = w, new_bytes_per_line = bytes_per_pixel * new_w;
	unsigned char *new_data = new unsigned char[calculate_data_size(new_w, new_h, new_bytes_per_line, color_model)];
	unsigned char **new_rows = new unsigned char*[new_h];
	for(int i = 0; i < new_h; i++)
		new_rows[i] = &new_data[new_bytes_per_line * i];

// Copy data
	for(int in_y = 0, out_x = 0; in_y < h; in_y++, out_x++)
	{
		for(int in_x = 0, out_y = new_h - 1; in_x < w; in_x++, out_y--)
		{
			for(int k = 0; k < bytes_per_pixel; k++)
			{
				new_rows[out_y][out_x * bytes_per_pixel + k] = 
					rows[in_y][in_x * bytes_per_pixel + k];
			}
		}
	}

// Swap frames
	clear_objects(0);
	data = new_data;
	rows = new_rows;
	bytes_per_line = new_bytes_per_line;
	w = new_w;
	h = new_h;
}

void VFrame::flip_vert(void)
{
	unsigned char *temp = new unsigned char[bytes_per_line];
	for(int i = 0, j = h - 1; i < j; i++, j--)
	{
		memcpy(temp, rows[j], bytes_per_line);
		memcpy(rows[j], rows[i], bytes_per_line);
		memcpy(rows[i], temp, bytes_per_line);
	}
	delete [] temp;
}



void VFrame::copy_from(VFrame *frame, int do_copy_pts)
{
	int w = MIN(this->w, frame->get_w());
	int h = MIN(this->h, frame->get_h());

	if(do_copy_pts)
		copy_pts(frame);

	switch(frame->color_model)
	{
	case BC_COMPRESSED:
		allocate_compressed_data(frame->compressed_size);
		memcpy(data, frame->data, frame->compressed_size);
		this->compressed_size = frame->compressed_size;
		break;

	case BC_YUV420P:
		memcpy(get_y(), frame->get_y(), w * h);
		memcpy(get_u(), frame->get_u(), w * h / 4);
		memcpy(get_v(), frame->get_v(), w * h / 4);
		break;

	case BC_YUV422P:
		memcpy(get_y(), frame->get_y(), w * h);
		memcpy(get_u(), frame->get_u(), w * h / 2);
		memcpy(get_v(), frame->get_v(), w * h / 2);
		break;

	default:
		memcpy(data, frame->data, calculate_data_size(w, 
			h, 
			-1, 
			frame->color_model));
		break;
	}
}


#define OVERLAY(type, max, components) \
{ \
	type **in_rows = (type**)src->get_rows(); \
	type **out_rows = (type**)get_rows(); \
	int in_w = src->get_w(); \
	int in_h = src->get_h(); \
 \
	for(int i = 0; i < in_h; i++) \
	{ \
		if(i + out_y1 >= 0 && i + out_y1 < h) \
		{ \
			type *src_row = in_rows[i]; \
			type *dst_row = out_rows[i + out_y1] + out_x1 * components; \
 \
			for(int j = 0; j < in_w; j++) \
			{ \
				if(j + out_x1 >= 0 && j + out_x1 < w) \
				{ \
					int opacity = src_row[3]; \
					int transparency = max - src_row[3]; \
					dst_row[0] = (transparency * dst_row[0] + opacity * src_row[0]) / max; \
					dst_row[1] = (transparency * dst_row[1] + opacity * src_row[1]) / max; \
					dst_row[2] = (transparency * dst_row[2] + opacity * src_row[2]) / max; \
					dst_row[3] = MAX(dst_row[3], src_row[3]); \
				} \
 \
				dst_row += components; \
				src_row += components; \
			} \
		} \
	} \
}


void VFrame::overlay(VFrame *src, 
		int out_x1, 
		int out_y1)
{
	switch(get_color_model())
	{
	case BC_RGBA8888:
		OVERLAY(unsigned char, 0xff, 4);
		break;
	}
}


void VFrame::get_scale_tables(int *column_table, int *row_table, 
			int in_x1, int in_y1, int in_x2, int in_y2,
			int out_x1, int out_y1, int out_x2, int out_y2)
{
	int y_out, i;
	float w_in = in_x2 - in_x1;
	float h_in = in_y2 - in_y1;
	int w_out = out_x2 - out_x1;
	int h_out = out_y2 - out_y1;

	float hscale = w_in / w_out;
	float vscale = h_in / h_out;

	for(i = 0; i < w_out; i++)
	{
		column_table[i] = (int)(hscale * i);
	}

	for(i = 0; i < h_out; i++)
	{
		row_table[i] = (int)(vscale * i) + in_y1;
	}
}

int VFrame::get_bytes_per_pixel(void)
{
	return bytes_per_pixel;
}

unsigned char** VFrame::get_rows(void)
{
	if(rows)
	{
		return rows;
	}
	return 0;
}

int VFrame::get_w(void)
{
	return w;
}

int VFrame::get_h(void)
{
	return h;
}

int VFrame::get_w_fixed(void)
{
	return w - 1;
}

int VFrame::get_h_fixed(void)
{
	return h - 1;
}

unsigned char* VFrame::get_y(void)
{
	return y;
}

unsigned char* VFrame::get_u(void)
{
	return u;
}

unsigned char* VFrame::get_v(void)
{
	return v;
}

void VFrame::set_number(long number)
{
	sequence_number = number;
}

long VFrame::get_number(void)
{
	return sequence_number;
}

void VFrame::clear_pts(void)
{
	pts = -1;
	source_pts = 0;
	layer = 0;
	duration = 0;
	frame_number = -1;
}

void VFrame::copy_pts(VFrame *frame)
{
	pts = frame->pts;
	source_pts = frame->source_pts;
	layer = frame->layer;
	duration = frame->duration;
	frame_number = frame->frame_number;
}

void VFrame::set_frame_number(framenum number)
{
	frame_number = number;
}

framenum VFrame::get_frame_number(void)
{
	return frame_number;
}

void VFrame::set_layer(int layer)
{
	this->layer = layer;
}

int VFrame::get_layer(void)
{
	return layer;
}

void VFrame::set_source_pts(ptstime pts)
{
	this->source_pts = pts;
}

ptstime VFrame::get_source_pts(void)
{
	return source_pts;
}

void VFrame::set_pts(ptstime pts)
{
	this->pts = pts;
}

ptstime VFrame::get_pts(void)
{
	return pts;
}

void VFrame::set_duration(ptstime duration)
{
	this->duration = duration;
}

ptstime VFrame::get_duration(void)
{
	return duration;
}

ptstime VFrame::next_pts()
{
	return pts + duration;
}

int VFrame::pts_in_frame(ptstime pts, ptstime accuracy)
{
	ptstime te = this->pts + this->duration;
	ptstime qe = pts + accuracy;
	ptstime limit = 0.5 * accuracy;

	if(qe < this->pts || pts > te)
		return 0;

	if((this->pts <= pts && qe < te) || pts <= this->pts && qe > te)
		return 1;

	if(pts < this->pts && qe < te && (this->pts - pts) < limit)
		return 1;

	if(pts > this->pts && qe > te && (te - pts) > limit)
		return 1;

	return 0;
}

void VFrame::push_prev_effect(const char *name)
{
	char *ptr;
	prev_effects.append(ptr = new char[strlen(name) + 1]);
	strcpy(ptr, name);
	if(prev_effects.total > MAX_STACK_ELEMENTS) prev_effects.remove_object(0);
}

void VFrame::pop_prev_effect(void)
{
	if(prev_effects.total)
		prev_effects.remove_object(prev_effects.last());
}

void VFrame::push_next_effect(const char *name)
{
	char *ptr;
	next_effects.append(ptr = new char[strlen(name) + 1]);
	strcpy(ptr, name);
	if(next_effects.total > MAX_STACK_ELEMENTS) next_effects.remove_object(0);
}

void VFrame::pop_next_effect(void)
{
	if(next_effects.total)
		next_effects.remove_object(next_effects.last());
}

const char* VFrame::get_next_effect(int number)
{
	if(!next_effects.total) return "";
	else
	if(number > next_effects.total - 1) number = next_effects.total - 1;

	return next_effects.values[next_effects.total - number - 1];
}

const char* VFrame::get_prev_effect(int number)
{
	if(!prev_effects.total) return "";
	else
	if(number > prev_effects.total - 1) number = prev_effects.total - 1;

	return prev_effects.values[prev_effects.total - number - 1];
}

BC_Hash* VFrame::get_params(void)
{
	return params;
}

void VFrame::clear_stacks(void)
{
	next_effects.remove_all_objects();
	prev_effects.remove_all_objects();
	delete params;
	params = new BC_Hash;
}

void VFrame::copy_params(VFrame *src)
{
	params->copy_from(src->params);
}

void VFrame::copy_stacks(VFrame *src)
{
	clear_stacks();

	for(int i = 0; i < src->next_effects.total; i++)
	{
		char *ptr;
		next_effects.append(ptr = new char[strlen(src->next_effects.values[i]) + 1]);
		strcpy(ptr, src->next_effects.values[i]);
	}
	for(int i = 0; i < src->prev_effects.total; i++)
	{
		char *ptr;
		prev_effects.append(ptr = new char[strlen(src->prev_effects.values[i]) + 1]);
		strcpy(ptr, src->prev_effects.values[i]);
	}

	params->copy_from(src->params);
}

int VFrame::equal_stacks(VFrame *src)
{
	for(int i = 0; i < src->next_effects.total && i < next_effects.total; i++)
	{
		if(strcmp(src->next_effects.values[i], next_effects.values[i])) return 0;
	}
	for(int i = 0; i < src->prev_effects.total && i < prev_effects.total; i++)
	{
		if(strcmp(src->prev_effects.values[i], prev_effects.values[i])) return 0;
	}
	if(!params->equivalent(src->params)) return 0;
	return 1;
}

void VFrame::calc_minmax8(unsigned char *buf, int len, 
		unsigned int &avg, int &min, int &max)
{
	max = avg = 0;
	min = 256;
	if(buf == 0)
		return;
	for(int i = 0; i < len; i++)
	{
		avg += buf[i];
		if(min > buf[i])
			min = buf[i];
		if(max < data[i])
			max = buf[i];
	}
	avg /= len;
}

void VFrame::calc_minmax8n(unsigned char *buf, int len, int pixlen,
		int *avg, int *min, int *max)
{
	int i, j, v;

	for(i = 0; i < pixlen; i++)
	{
		max[i] = avg[i] = 0;
		min[i] = 256;
	}
	for(i = 0; i < len; i++)
	{
		for(j = 0; j < pixlen; j++)
		{
			v = *buf++;
			avg[j] += v;
			if(min[j] > v)
				min[j] = v;
			if(max[j] < v)
				max[j] = v;
		}
	}
	for(i = 0; i < pixlen; i++)
	{
		avg[i] /= len;
	}
}

void VFrame::calc_minmax16(uint16_t *buf, int len, int pixlen,
		uint64_t *avg, uint64_t *min, uint64_t *max)
{
	int i, j, v;

	for(i = 0; i < pixlen; i++)
	{
		max[i] = avg[i] = 0;
		min[i] = 256;
	}
	for(i = 0; i < len; i++)
	{
		for(j = 0; j < pixlen; j++)
		{
			v = *buf++;
			avg[j] += v;
			if(min[j] > v)
				min[j] = v;
			if(max[j] < v)
				max[j] = v;
		}
	}
	for(i = 0; i < pixlen; i++)
	{
		avg[i] /= len;
	}
}

void VFrame::calc_minmaxfl(float *buf, int len, int pixlen,
		float *avg, float *min, float *max)
{
	int i, j;
	float v;

	for(i = 0; i < pixlen; i++)
	{
		max[i] = avg[i] = 0;
		min[i] = 256;
	}
	for(i = 0; i < len; i++)
	{
		for(j = 0; j < pixlen; j++)
		{
			v = *buf++;
			avg[j] += v;
			if(min[j] > v)
				min[j] = v;
			if(max[j] < v)
				max[j] = v;
		}
	}
	for(i = 0; i < pixlen; i++)
	{
		avg[i] /= len;
	}
}

void VFrame::dump(int minmax)
{
	char strbuf[128];

	printf("VFrame %p dump\n", this);
	cmodel_to_text(strbuf, color_model);
	printf("    pts %.3f, duration %.3f src_pts %.3f frame %d layer %d\n", 
		pts, duration, source_pts, frame_number, layer);
	printf("    Size %dx%d, cmodel %s offsets %ld %ld %ld\n", w, h, 
		strbuf, y_offset, u_offset, v_offset);
	printf("    data:%p rows: %p y:%p, u:%p, v:%p\n", data, rows,
		y, u, v);
	printf("    compressed size %ld, compressed_allocated %ld\n",
		compressed_size, compressed_allocated);
	if(minmax)
	{
		int min, max;
		unsigned int avg;
		int amin[4], amax[4], aavg[4];
		uint64_t lmin[4], lmax[4], lavg[4];
		float fmin[4], fmax[4], favg[4];
		int anum = 0;
		int lnum = 0;
		int fnum = 0;

		switch(color_model)
		{
		case BC_COMPRESSED:
			calc_minmax8(data, compressed_size, avg, min, max);
			printf("    avg %d min %d max %d\n", avg, min, max);
			break;
		case BC_YUV420P:
			calc_minmax8(get_y(), w * h, avg, min, max);
			printf("    y: avg %d min %d max %d\n", avg, min, max);
			calc_minmax8(get_u(), w * h / 4, avg, min, max);
			printf("    u: avg %d min %d max %d\n", avg, min, max);
			calc_minmax8(get_v(), w * h / 4, avg, min, max);
			printf("    v: avg %d min %d max %d\n", avg, min, max);
			break;
		case BC_YUV422P:
			calc_minmax8(get_y(), w * h, avg, min, max);
			printf("    y: avg %d min %d max %d\n", avg, min, max);
			calc_minmax8(get_u(), w * h / 2, avg, min, max);
			printf("    u: avg %d min %d max %d\n", avg, min, max);
			calc_minmax8(get_v(), w * h / 2, avg, min, max);
			printf("    v: avg %d min %d max %d\n", avg, min, max);
			break;
		case BC_RGB888:
		case BC_YUV888:
		case BC_VYU888:
		case BC_BGR888:
			calc_minmax8n(data, w * h, 3, aavg, amin, amax);
			anum = 3;
			break;
		case BC_ARGB8888:
		case BC_ABGR8888:
		case BC_RGBA8888:
		case BC_BGR8888:
		case BC_YUVA8888:
		case BC_UYVA8888:
			calc_minmax8n(data, w * h, 4, aavg, amin, amax);
			anum = 4;
			break;
		case BC_YUV161616:
		case BC_RGB161616:
			calc_minmax16((uint16_t *)data, w * h, 3, lavg, lmin, lmax);
			lnum = 3;
			break;
		case BC_YUVA16161616:
		case BC_RGBA16161616:
			calc_minmax16((uint16_t *)data, w * h, 4, lavg, lmin, lmax);
			lnum = 4;
			break;
		case BC_RGB_FLOAT:
			calc_minmaxfl((float *)data, w * h, 3, favg, fmin, fmax);
			fnum = 3;
			break;
		case BC_RGBA_FLOAT:
			calc_minmaxfl((float *)data, w * h, 4, favg, fmin, fmax);
			fnum = 4;
			break;
		default:
			calc_minmax8(data, calculate_data_size(w, h, bytes_per_line, color_model),
				avg, min, max);
			printf("    avg %d min %d max %d\n", avg, min, max);
			break;
		}
		for(int i = 0; i < anum; i++)
			printf("    l:%d avg %d min %d max %d\n", i, aavg[i], amin[i], amax[i]);
		for(int i = 0; i < lnum; i++)
			printf("    l:%d avg %lld min %lld max %lld\n", i, lavg[i], lmin[i], lmax[i]);
		for(int i = 0; i < fnum; i++)
			printf("    l:%d avg %.3f min %.3f max %.3f\n", i, favg[i], fmin[i], fmax[i]);
	}
}

void VFrame::dump_file(const char *filename)
{
	FILE *fp;
	int i;

	if(w == 0 || h == 0)
		return;
	if(rows)
	{
		if(fp = fopen(filename, "wb"))
		{
			for(i = 0; i < h; i++)
				fwrite(rows[i], w, 1, fp);
			fclose(fp);
		}
		return;
	}
	if(y)
	{
		if(fp = fopen(filename, "wb"))
		{
			fwrite(y, w * h, 1, fp);
			fclose(fp);
		}
		return;
	}
}

void VFrame::dump_stacks(void)
{
	printf("VFrame::dump_stacks\n");
	printf("        next_effects:\n");
	for(int i = next_effects.total - 1; i >= 0; i--)
		printf("                %s\n", next_effects.values[i]);
	printf("        prev_effects:\n");
	for(int i = prev_effects.total - 1; i >= 0; i--)
		printf("                %s\n", prev_effects.values[i]);
}

void VFrame::dump_params(void)
{
	params->dump();
}
