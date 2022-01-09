// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include <inttypes.h>
#include <png.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "bchash.h"
#include "bcsignals.h"
#include "bcwindowbase.h"
#include "clip.h"
#include "colormodels.h"
#include "vframe.h"

// Data padding for CPU vector extensions
#define OVERALLOC 32

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
	status = frame.get_status();
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
	return (src->get_color_model() == color_model &&
		src->get_w() == w &&
		src->get_h() == h &&
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
	status = 0;
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
	return calculate_data_size(w, h, bytes_per_line, color_model) - OVERALLOC;
}

size_t VFrame::calculate_data_size(int w, int h, int bytes_per_line, int color_model)
{
	return ColorModels::calculate_datasize(w, h, bytes_per_line, color_model) +
		OVERALLOC;
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
	this->bytes_per_pixel = ColorModels::calculate_pixelsize(color_model);
	this->y_offset = this->u_offset = this->v_offset = 0;

	if(bytes_per_line >= 0)
	{
		this->bytes_per_line = bytes_per_line;
	}
	else
		this->bytes_per_line = (this->bytes_per_pixel * w + 15) & ~15;

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
	delete_row_ptrs();
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
		type *row = (type*)get_row_ptr(i); \
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
		ZERO_YUV(4, uint16_t, 0xffff);
		break;

	case BC_AYUV16161616:
		for(int i = 0; i < h; i++)
		{
			uint16_t *row = (uint16_t*)get_row_ptr(i);
			for(int j = 0; j < w; j++)
			{
				row[j * 4] = 0;
				row[j * 4 + 1] = 0;
				row[j * 4 + 2] = 0x8000;
				row[j * 4 + 3] = 0x8000;
			}
		}
		break;

	default:
		memset(data, 0, calculate_data_size(w, h, bytes_per_line, color_model));
		break;
	}
	status = 0;
}

void VFrame::flip_vert(void)
{
	unsigned char *temp = new unsigned char[bytes_per_line];
	for(int i = 0, j = h - 1; i < j; i++, j--)
	{
		memcpy(temp, get_row_ptr(j), bytes_per_line);
		memcpy(get_row_ptr(j), get_row_ptr(i), bytes_per_line);
		memcpy(get_row_ptr(i), temp, bytes_per_line);
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
	status = frame->get_status();
}

void VFrame::copy_from(VFrame *frame, int do_copy_pts)
{
	if(frame == this)
		return;

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
	status = frame->get_status();
}

int VFrame::get_bytes_per_pixel(void)
{
	return bytes_per_pixel;
}

unsigned char *VFrame::get_row_ptr(int num)
{
	if(num < 0)
		num = 0;
	else if(num >= h)
		num = h - 1;
	return data + num * bytes_per_line;
}

unsigned char** VFrame::get_rows(void)
{
	if(rows)
		return rows;

	switch(color_model)
	{
	case BC_YUV420P:
	case BC_YUV411P:
	case BC_YUV422P:
	case BC_YUV444P:
		break;
	default:
		rows = new unsigned char*[h];
		for(int i = 0; i < h; i++)
			rows[i] = &data[i * bytes_per_line];
		break;
	}
	return rows;
}

void VFrame::delete_row_ptrs()
{
	delete [] rows;
	rows = 0;
}

int VFrame::get_w(void)
{
	return w;
}

int VFrame::get_h(void)
{
	return h;
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

int VFrame::get_status()
{
	return status;
}

void VFrame::clear_status()
{
	status = 0;
}

void VFrame::set_transparent()
{
	status |= VFRAME_TRANSPARENT;
}

void VFrame::clear_transparent()
{
	status &= ~VFRAME_TRANSPARENT;
}

int VFrame::is_transparent()
{
	return status & VFRAME_TRANSPARENT;
}

void VFrame::merge_status(VFrame *that)
{
	status |= that->status;
}

int VFrame::pts_in_frame_source(ptstime pts, ptstime accuracy)
{
	pts += accuracy;
	if(pts < source_pts || pts > source_pts + duration)
		return 0;
	return 1;
}

int VFrame::pts_in_frame(ptstime pts, ptstime accuracy)
{
	pts += accuracy;
	if(pts < this->pts || pts > this->pts + duration)
		return 0;
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

void VFrame::dump(int indent, int minmax)
{
	const char *st;

	printf("%*sVFrame %p dump:\n", indent, "", this);
	indent += 2;
	printf("%*spts %.3f, duration %.3f src_pts %.3f frame %d layer %d\n", indent, "",
		pts, duration, source_pts, frame_number, layer);
	printf("%*s[%dx%d], '%s' bytes/line: %d offsets: %d %d %d\n", indent, "", w, h,
		ColorModels::name(color_model), bytes_per_line, y_offset, u_offset, v_offset);
	printf("%*sdata:%p rows: %p y:%p, u:%p, v:%p%s\n", indent, "", data, rows,
		y, u, v, shared ? " shared" : "");
	printf("%*scompressed: %d, allocated: %d SAR: %.2f status: %#x\n", indent, "",
		compressed_size, compressed_allocated, pixel_aspect, status);

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
			printf("%*savg %d min %d max %d\n", indent, "", avg, min, max);
			break;
		case BC_YUV420P:
			calc_minmax8(get_y(), w * h, avg, min, max);
			printf("%*sy: avg %d min %d max %d\n", indent, "", avg, min, max);
			calc_minmax8(get_u(), w * h / 4, avg, min, max);
			printf("%*su: avg %d min %d max %d\n", indent, "", avg, min, max);
			calc_minmax8(get_v(), w * h / 4, avg, min, max);
			printf("%*sv: avg %d min %d max %d\n", indent, "", avg, min, max);
			break;
		case BC_YUV422P:
			calc_minmax8(get_y(), w * h, avg, min, max);
			printf("%*sy: avg %d min %d max %d\n", indent, "", avg, min, max);
			calc_minmax8(get_u(), w * h / 2, avg, min, max);
			printf("%*su: avg %d min %d max %d\n", indent, "", avg, min, max);
			calc_minmax8(get_v(), w * h / 2, avg, min, max);
			printf("%*sv: avg %d min %d max %d\n", indent, "", avg, min, max);
			break;
		case BC_YUV444P:
			calc_minmax8(get_y(), w * h, avg, min, max);
			printf("%*sy: avg %d min %d max %d\n", indent, "", avg, min, max);
			calc_minmax8(get_u(), w * h, avg, min, max);
			printf("%*su: avg %d min %d max %d\n", indent, "",avg, min, max);
			calc_minmax8(get_v(), w * h, avg, min, max);
			printf("%*sv: avg %d min %d max %d\n", indent, "", avg, min, max);
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
			printf("%*savg %d min %d max %d\n", indent, "", avg, min, max);
			break;
		}
		for(int i = 0; i < anum; i++)
			printf("%*sl:%d avg %d min %d max %d\n", indent, "", i, aavg[i], amin[i], amax[i]);
		for(int i = 0; i < lnum; i++)
			printf("%*sl:%d avg %" PRId64 " min %" PRId64 " max %" PRId64 "\n", indent, "", i, lavg[i], lmin[i], lmax[i]);
		for(int i = 0; i < fnum; i++)
			printf("%*sl:%d avg %.3f min %.3f max %.3f\n", indent, "", i, favg[i], fmin[i], fmax[i]);
	}
}

void VFrame::dump_file(const char *filename)
{
	FILE *fp;
	int i;

	if(w == 0 || h == 0)
		return;
	if(y)
	{
		if(fp = fopen(filename, "wb"))
		{
			fwrite(y, w * h, 1, fp);
			fclose(fp);
		}
		else
			goto err;
		return;
	}
	if(fp = fopen(filename, "wb"))
	{
		for(i = 0; i < h; i++)
			fwrite(get_row_ptr(i), w,
				ColorModels::calculate_pixelsize(color_model), fp);
		fclose(fp);
	}
	return;
err:
	printf("VFrame::dump_file: Failed to create %s\n", filename);
}
