
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

#include <inttypes.h>
#include <png.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "bchash.h"
#include "bcsignals.h"
#include "bcsynchronous.h"
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
	reset_parameters();
	read_png(png_data);
}

VFrame::VFrame(VFrame &frame)
{
	reset_parameters();
	allocate_data(0, 0, 0, 0, frame.w, frame.h, frame.color_model, frame.bytes_per_line);
	memcpy(data, frame.data, bytes_per_line * h);
	copy_pts(&frame);
}

VFrame::VFrame(unsigned char *data, 
	int w, 
	int h, 
	int color_model,
	int bytes_per_line)
{
	reset_parameters();
	allocate_data(data, 0, 0, 0, w, h, color_model, bytes_per_line);
}

VFrame::VFrame(unsigned char *data, 
		int y_offset,
		int u_offset,
		int v_offset,
		int w, 
		int h, 
		int color_model, 
		int bytes_per_line)
{
	reset_parameters();
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
	reset_parameters();
	this->color_model = BC_COMPRESSED;
}

VFrame::~VFrame()
{
	clear_objects();
}

int VFrame::equivalent(VFrame *src)
{
	return (src->get_color_model() == get_color_model() &&
		src->get_w() == get_w() &&
		src->get_h() == get_h() &&
		src->bytes_per_line == bytes_per_line);
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


void VFrame::reset_parameters()
{
	field2_offset = -1;
	shared = 0;
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
	clear_pts();
	pixel_aspect = 0;
}

void VFrame::clear_objects()
{
// Delete data
	if(!shared)
	{
		if(data) delete [] data;
		data = 0;
	}

// Delete row pointers
	delete [] rows;
	rows = 0;
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

int VFrame::calculate_bytes_per_pixel(int color_model)
{
	return ColorModels::calculate_pixelsize(color_model);
}

int VFrame::get_bytes_per_line()
{
	return bytes_per_line;
}

int VFrame::get_pixels_per_line()
{
	return bytes_per_line / bytes_per_pixel;
}

size_t VFrame::get_data_size()
{
	return calculate_data_size(w, h, bytes_per_line, color_model) - 4;
}

size_t VFrame::calculate_data_size(int w, int h, int bytes_per_line, int color_model)
{
	return ColorModels::calculate_datasize(w, h, bytes_per_line, color_model);
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

	case BC_YUV444P:
		if(!this->v_offset)
		{
			this->y_offset = 0;
			this->u_offset = w * h;
			this->v_offset = w * h + w * h;
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
	int y_offset,
	int u_offset,
	int v_offset,
	int w, 
	int h, 
	int color_model, 
	int bytes_per_line)
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
		this->bytes_per_line = (this->bytes_per_pixel * w + 15) & ~15;

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
		int y_offset,
		int u_offset,
		int v_offset)
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
	clear_objects();
	shared = 1;
	this->data = data;
	this->compressed_allocated = data_allocated;
	this->compressed_size = data_size;
}


// Reallocate uncompressed buffer with or without alpha
void VFrame::reallocate(unsigned char *data, 
		int y_offset,
		int u_offset,
		int v_offset,
		int w, 
		int h, 
		int color_model, 
		int bytes_per_line)
{
	clear_objects();
	reset_parameters();
	allocate_data(data, 
		y_offset, 
		u_offset, 
		v_offset, 
		w, 
		h, 
		color_model, 
		bytes_per_line);
}

void VFrame::allocate_compressed_data(int bytes)
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

int VFrame::get_compressed_allocated()
{
	return compressed_allocated;
}

int VFrame::get_compressed_size()
{
	return compressed_size;
}

void VFrame::set_compressed_size(int size)
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
	case BC_AYUV16161616:
		ZERO_YUV(4, uint16_t, 0xffff);
		break;

	default:
		memset(data, 0, calculate_data_size(w, h, bytes_per_line, color_model));
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
	clear_objects();
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
	clear_objects();
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

void VFrame::transfer_from(VFrame *frame)
{
	ColorModels::transfer_sws(data,
		frame->get_data(),
		y, u, v,
		frame->get_y(), frame->get_u(), frame->get_v(),
		frame->get_w(), frame->get_h(), w, h,
		frame->get_color_model(), color_model,
		frame->get_bytes_per_line(),
		bytes_per_line);
}

void VFrame::copy_from(VFrame *frame, int do_copy_pts)
{
	int w = MIN(this->w, frame->get_w());
	int h = MIN(this->h, frame->get_h());
	int bytes = MIN(bytes_per_line, frame->get_bytes_per_line());

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
		memcpy(data, frame->data, calculate_data_size(w, h,
			bytes, frame->color_model));
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

double VFrame::get_pixel_aspect()
{
	return pixel_aspect;
}

void VFrame::set_pixel_aspect(double aspect)
{
	pixel_aspect = aspect;
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

ptstime VFrame::next_source_pts()
{
	return source_pts + duration;
}

int VFrame::pts_in_frame_source(ptstime pts, ptstime accuracy)
{
	ptstime te = this->source_pts + this->duration;
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
		if(max < buf[i])
			max = buf[i];
	}
	avg /= len;
}

void VFrame::calc_minmax8n(unsigned char *buf, int pixlen,
		int width, int height, int bytes_per_line,
		int *avg, int *min, int *max)
{
	int i, j, v, r;
	unsigned char *rp;

	for(i = 0; i < pixlen; i++)
	{
		max[i] = avg[i] = 0;
		min[i] = 256;
	}
	for(r = 0; r < height; r++)
	{
		rp = buf + r * bytes_per_line;

		for(i = 0; i < width; i++)
		{
			for(j = 0; j < pixlen; j++)
			{
				v = *rp++;
				avg[j] += v;
				if(min[j] > v)
					min[j] = v;
				if(max[j] < v)
					max[j] = v;
			}
		}
	}
	for(i = 0; i < pixlen; i++)
	{
		avg[i] /= width * height;
	}
}

void VFrame::calc_minmax16(uint16_t *buf, int pixlen,
		int width, int height, int bytes_per_line,
		uint64_t *avg, uint64_t *min, uint64_t *max)
{
	int i, j, v, r;
	uint16_t *rp;

	for(i = 0; i < pixlen; i++)
	{
		max[i] = avg[i] = 0;
		min[i] = 0x10000;
	}
	for(r = 0; r < height; r++)
	{
		rp = (uint16_t *)(((char *)buf) + r * bytes_per_line);

		for(i = 0; i < width; i++)
		{
			for(j = 0; j < pixlen; j++)
			{
				v = *rp++;
				avg[j] += v;
				if(min[j] > v)
					min[j] = v;
				if(max[j] < v)
					max[j] = v;
			}
		}
	}
	for(i = 0; i < pixlen; i++)
	{
		avg[i] /= width * height;
	}
}

void VFrame::calc_minmaxfl(float *buf, int pixlen,
		int width, int height, int bytes_per_line,
		float *avg, float *min, float *max)
{
	int i, j, r;
	float v;
	float *rp;

	for(i = 0; i < pixlen; i++)
	{
		max[i] = avg[i] = 0;
		min[i] = 256;
	}

	for(r = 0; r < height; r++)
	{
		rp = (float *)(((char *)buf) + r * bytes_per_line);

		for(i = 0; i < width; i++)
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
	}
	for(i = 0; i < pixlen; i++)
	{
		avg[i] /= width * height;
	}
}

void VFrame::dump(int minmax)
{
	const char *st;

	printf("VFrame %p dump\n", this);
	printf("    pts %.3f, duration %.3f src_pts %.3f frame %d layer %d\n", 
		pts, duration, source_pts, frame_number, layer);
	printf("    Size %dx%d, cmodel %s bytes/line %d offsets %d %d %d\n", w, h,
		ColorModels::name(color_model), bytes_per_line, y_offset, u_offset, v_offset);
	printf("    data:%p rows: %p y:%p, u:%p, v:%p%s\n", data, rows,
		y, u, v, shared ? " shared" : "");
	printf("    compressed %d, allocated %d pix apect %.2f\n",
		compressed_size, compressed_allocated, pixel_aspect);

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
		case BC_YUV444P:
			calc_minmax8(get_y(), w * h, avg, min, max);
			printf("    y: avg %d min %d max %d\n", avg, min, max);
			calc_minmax8(get_u(), w * h, avg, min, max);
			printf("    u: avg %d min %d max %d\n", avg, min, max);
			calc_minmax8(get_v(), w * h, avg, min, max);
			printf("    v: avg %d min %d max %d\n", avg, min, max);
			break;
		case BC_RGB888:
		case BC_YUV888:
		case BC_BGR888:
			calc_minmax8n(data, 3, w, h, bytes_per_line,
				aavg, amin, amax);
			anum = 3;
			break;
		case BC_ARGB8888:
		case BC_ABGR8888:
		case BC_RGBA8888:
		case BC_BGR8888:
		case BC_YUVA8888:
			calc_minmax8n(data, 4, w, h, bytes_per_line,
				aavg, amin, amax);
			anum = 4;
			break;
		case BC_YUV161616:
		case BC_RGB161616:
			calc_minmax16((uint16_t *)data, 3, w, h, bytes_per_line,
				lavg, lmin, lmax);
			lnum = 3;
			break;
		case BC_YUVA16161616:
		case BC_AYUV16161616:
		case BC_RGBA16161616:
			calc_minmax16((uint16_t *)data, 4, w, h, bytes_per_line,
				lavg, lmin, lmax);
			lnum = 4;
			break;
		case BC_RGB_FLOAT:
			calc_minmaxfl((float *)data, 3, w, h, bytes_per_line,
				favg, fmin, fmax);
			fnum = 3;
			break;
		case BC_RGBA_FLOAT:
			calc_minmaxfl((float *)data, 4, w, h, bytes_per_line,
				favg, fmin, fmax);
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
			printf("    l:%d avg %" PRId64 " min %" PRId64 " max %" PRId64 "\n", i, lavg[i], lmin[i], lmax[i]);
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
